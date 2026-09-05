#include "order_book.h"

#include <algorithm>
#include <functional>
#include <limits>

namespace
{
template<typename Levels, typename Compare>
auto find_level(Levels& levels, std::int64_t price_ticks, Compare compare)
{
    return std::lower_bound(
        levels.begin(), levels.end(), price_ticks,
        [compare](const auto& level, std::int64_t price)
        {
            return compare(level.price_ticks, price);
        });
}

template<typename Levels, typename Compare>
auto existing_level(Levels& levels, std::int64_t price_ticks, Compare compare)
{
    const auto level = find_level(levels, price_ticks, compare);
    return level != levels.end() && level->price_ticks == price_ticks
        ? level
        : levels.end();
}

template<typename Level>
PriceLevel level_view(const Level& level) noexcept
{
    return PriceLevel{level.price_ticks, level.quantity, level.order_count};
}
}

OrderBook::OrderBook(
    std::uint32_t instrument_id,
    std::size_t expected_order_count)
    : _instrument_id{instrument_id}
{
    _bids.reserve(64);
    _asks.reserve(64);
    _orders.reserve(expected_order_count);
}

ApplyResult OrderBook::apply(const MarketDataEvent& event)
{
    if (event.instrument_id != _instrument_id)
    {
        return ApplyResult::InstrumentMismatch;
    }
    if (event.order_id == 0)
    {
        return ApplyResult::InvalidOrderId;
    }

    switch (event.action)
    {
    case Action::Add:
        return add(event);
    case Action::Update:
        return update(event);
    case Action::Delete:
        return remove(event);
    }

    return ApplyResult::OrderNotFound;
}

ApplyResult OrderBook::add(const MarketDataEvent& event)
{
    if (event.quantity == 0)
    {
        return ApplyResult::InvalidQuantity;
    }
    if (_orders.contains(event.order_id))
    {
        return ApplyResult::OrderAlreadyExists;
    }

    auto& levels = event.side == Side::Buy ? _bids : _asks;
    auto level = event.side == Side::Buy
        ? find_level(levels, event.price_ticks, std::greater<>{})
        : find_level(levels, event.price_ticks, std::less<>{});

    if (level == levels.end() || level->price_ticks != event.price_ticks)
    {
        level = levels.insert(level, LevelState{.price_ticks = event.price_ticks});
    }
    if (event.quantity > std::numeric_limits<std::uint64_t>::max() - level->quantity)
    {
        if (level->order_count == 0)
        {
            levels.erase(level);
        }
        return ApplyResult::QuantityOverflow;
    }

    const auto previous_order_id = level->last_order_id;
    _orders.emplace(event.order_id, OrderNode{
        .price_ticks = event.price_ticks,
        .quantity = event.quantity,
        .level_index = static_cast<std::size_t>(level - levels.begin()),
        .side = event.side,
        .previous = previous_order_id
    });

    if (previous_order_id != 0)
    {
        _orders.at(previous_order_id).next = event.order_id;
    }
    else
    {
        level->first_order_id = event.order_id;
    }
    level->last_order_id = event.order_id;
    level->quantity += event.quantity;
    ++level->order_count;
    return ApplyResult::Applied;
}

ApplyResult OrderBook::update(const MarketDataEvent& event)
{
    if (event.quantity == 0)
    {
        return ApplyResult::InvalidQuantity;
    }

    const auto order = _orders.find(event.order_id);
    if (order == _orders.end())
    {
        return ApplyResult::OrderNotFound;
    }
    if (order->second.side != event.side
        || order->second.price_ticks != event.price_ticks)
    {
        return ApplyResult::OrderAttributesMismatch;
    }

    auto& levels = event.side == Side::Buy ? _bids : _asks;
    const auto hinted_level_index = order->second.level_index;
    const auto hint_is_valid = hinted_level_index < levels.size()
        && levels[hinted_level_index].price_ticks == event.price_ticks;
    auto level = hint_is_valid
        ? levels.begin() + static_cast<Levels::difference_type>(hinted_level_index)
        : event.side == Side::Buy
            ? existing_level(levels, event.price_ticks, std::greater<>{})
            : existing_level(levels, event.price_ticks, std::less<>{});

    const auto old_quantity = order->second.quantity;
    if (event.quantity > old_quantity
        && event.quantity - old_quantity
            > std::numeric_limits<std::uint64_t>::max() - level->quantity)
    {
        return ApplyResult::QuantityOverflow;
    }

    if (!hint_is_valid)
    {
        order->second.level_index =
            static_cast<std::size_t>(level - levels.begin());
    }
    level->quantity = level->quantity - old_quantity + event.quantity;
    order->second.quantity = event.quantity;
    return ApplyResult::Applied;
}

ApplyResult OrderBook::remove(const MarketDataEvent& event)
{
    if (event.quantity != 0)
    {
        return ApplyResult::InvalidQuantity;
    }

    const auto order = _orders.find(event.order_id);
    if (order == _orders.end())
    {
        return ApplyResult::OrderNotFound;
    }
    if (order->second.side != event.side
        || order->second.price_ticks != event.price_ticks)
    {
        return ApplyResult::OrderAttributesMismatch;
    }

    auto& levels = event.side == Side::Buy ? _bids : _asks;
    auto level = event.side == Side::Buy
        ? existing_level(levels, event.price_ticks, std::greater<>{})
        : existing_level(levels, event.price_ticks, std::less<>{});

    const auto previous = order->second.previous;
    const auto next = order->second.next;
    if (previous != 0)
    {
        _orders.at(previous).next = next;
    }
    else
    {
        level->first_order_id = next;
    }
    if (next != 0)
    {
        _orders.at(next).previous = previous;
    }
    else
    {
        level->last_order_id = previous;
    }

    level->quantity -= order->second.quantity;
    --level->order_count;
    _orders.erase(order);
    if (level->order_count == 0)
    {
        levels.erase(level);
    }
    return ApplyResult::Applied;
}

std::uint32_t OrderBook::instrument_id() const noexcept
{
    return _instrument_id;
}

std::optional<PriceLevel> OrderBook::best_bid() const noexcept
{
    return _bids.empty() ? std::nullopt : std::optional{level_view(_bids.front())};
}

std::optional<PriceLevel> OrderBook::best_ask() const noexcept
{
    return _asks.empty() ? std::nullopt : std::optional{level_view(_asks.front())};
}

std::optional<PriceLevel> OrderBook::level_at(
    Side side,
    std::size_t depth) const noexcept
{
    const auto& levels = side == Side::Buy ? _bids : _asks;
    return depth < levels.size()
        ? std::optional{level_view(levels[depth])}
        : std::nullopt;
}

std::optional<std::uint64_t> OrderBook::quantity_at(
    Side side,
    std::int64_t price_ticks) const noexcept
{
    const auto& levels = side == Side::Buy ? _bids : _asks;
    const auto level = side == Side::Buy
        ? existing_level(levels, price_ticks, std::greater<>{})
        : existing_level(levels, price_ticks, std::less<>{});
    return level == levels.end()
        ? std::nullopt
        : std::optional{level->quantity};
}

std::optional<OrderView> OrderBook::order(OrderId order_id) const noexcept
{
    const auto found = _orders.find(order_id);
    if (found == _orders.end())
    {
        return std::nullopt;
    }
    return OrderView{order_id, found->second.price_ticks,
                     found->second.quantity, found->second.side};
}

std::optional<OrderView> OrderBook::first_order_at(
    Side side,
    std::int64_t price_ticks) const noexcept
{
    const auto& levels = side == Side::Buy ? _bids : _asks;
    const auto level = side == Side::Buy
        ? existing_level(levels, price_ticks, std::greater<>{})
        : existing_level(levels, price_ticks, std::less<>{});
    return level == levels.end() ? std::nullopt : order(level->first_order_id);
}

std::size_t OrderBook::order_count() const noexcept
{
    return _orders.size();
}

std::size_t OrderBook::order_count_at(
    Side side,
    std::int64_t price_ticks) const noexcept
{
    const auto& levels = side == Side::Buy ? _bids : _asks;
    const auto level = side == Side::Buy
        ? existing_level(levels, price_ticks, std::greater<>{})
        : existing_level(levels, price_ticks, std::less<>{});
    return level == levels.end() ? 0 : level->order_count;
}

std::size_t OrderBook::level_count(Side side) const noexcept
{
    return side == Side::Buy ? _bids.size() : _asks.size();
}

bool OrderBook::empty() const noexcept
{
    return _orders.empty();
}
