#include "order_book.h"

namespace
{
template<typename Levels>
ApplyResult apply_to_levels(Levels& levels, const MarketDataEvent& event)
{
    const auto level = levels.find(event.price_ticks);

    switch (event.action)
    {
    case Action::Add:
        if (event.quantity == 0)
        {
            return ApplyResult::InvalidQuantity;
        }
        if (level != levels.end())
        {
            return ApplyResult::LevelAlreadyExists;
        }
        levels.emplace(event.price_ticks, event.quantity);
        return ApplyResult::Applied;

    case Action::Update:
        if (event.quantity == 0)
        {
            return ApplyResult::InvalidQuantity;
        }
        if (level == levels.end())
        {
            return ApplyResult::LevelNotFound;
        }
        level->second = event.quantity;
        return ApplyResult::Applied;

    case Action::Delete:
        if (event.quantity != 0)
        {
            return ApplyResult::InvalidQuantity;
        }
        if (level == levels.end())
        {
            return ApplyResult::LevelNotFound;
        }
        levels.erase(level);
        return ApplyResult::Applied;
    }

    return ApplyResult::LevelNotFound;
}

template<typename Levels>
std::optional<PriceLevel> best_level(const Levels& levels) noexcept
{
    if (levels.empty())
    {
        return std::nullopt;
    }

    const auto& [price_ticks, quantity] = *levels.begin();
    return PriceLevel{price_ticks, quantity};
}

template<typename Levels>
std::optional<std::uint64_t> find_quantity(
    const Levels& levels,
    std::int64_t price_ticks) noexcept
{
    const auto level = levels.find(price_ticks);
    if (level == levels.end())
    {
        return std::nullopt;
    }
    return level->second;
}
}

OrderBook::OrderBook(std::uint32_t instrument_id)
    : _instrument_id{instrument_id}
{
}

ApplyResult OrderBook::apply(const MarketDataEvent& event)
{
    if (event.instrument_id != _instrument_id)
    {
        return ApplyResult::InstrumentMismatch;
    }

    return event.side == Side::Buy
        ? apply_to_levels(_bids, event)
        : apply_to_levels(_asks, event);
}

std::uint32_t OrderBook::instrument_id() const noexcept
{
    return _instrument_id;
}

std::optional<PriceLevel> OrderBook::best_bid() const noexcept
{
    return best_level(_bids);
}

std::optional<PriceLevel> OrderBook::best_ask() const noexcept
{
    return best_level(_asks);
}

std::optional<std::uint64_t> OrderBook::quantity_at(
    Side side,
    std::int64_t price_ticks) const noexcept
{
    return side == Side::Buy
        ? find_quantity(_bids, price_ticks)
        : find_quantity(_asks, price_ticks);
}

std::size_t OrderBook::level_count(Side side) const noexcept
{
    return side == Side::Buy ? _bids.size() : _asks.size();
}

bool OrderBook::empty() const noexcept
{
    return _bids.empty() && _asks.empty();
}
