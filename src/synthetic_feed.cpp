#include "synthetic_feed.h"

SyntheticFeed::SyntheticFeed(std::uint64_t seed)
    : _rng{seed},
      _price_offset_distribution{-20, 20},
      _quantity_distribution{1, 1000},
      _side_distribution{0, 1},
      _action_distribution{50, 35, 15}
{
    _active_orders.reserve(MaxActiveOrders);
}

MarketDataEvent SyntheticFeed::next()
{
    auto action = static_cast<Action>(_action_distribution(_rng));
    if (_active_orders.empty())
    {
        action = Action::Add;
    }
    else if (action == Action::Add && _active_orders.size() == MaxActiveOrders)
    {
        action = Action::Update;
    }

    MarketDataEvent event{
        .timestamp_ns = _timestamp_ns,
        .instrument_id = InstrumentId,
        .action = action
    };

    if (action == Action::Add)
    {
        event.order_id = _next_order_id++;
        event.price_ticks = BasePrice + _price_offset_distribution(_rng) * TickSize;
        event.quantity = _quantity_distribution(_rng);
        event.side = _side_distribution(_rng) == 0 ? Side::Buy : Side::Sell;

        _active_orders.push_back(ActiveOrder{
            event.order_id,
            event.price_ticks,
            event.quantity,
            event.side
        });
    }
    else
    {
        std::uniform_int_distribution<std::size_t> order_distribution{
            0, _active_orders.size() - 1};
        const auto order_index = order_distribution(_rng);
        auto& active_order = _active_orders[order_index];

        event.order_id = active_order.order_id;
        event.price_ticks = active_order.price_ticks;
        event.side = active_order.side;

        if (action == Action::Update)
        {
            event.quantity = _quantity_distribution(_rng);
            active_order.quantity = event.quantity;
        }
        else
        {
            event.quantity = 0;
            active_order = _active_orders.back();
            _active_orders.pop_back();
        }
    }

    _timestamp_ns += 100;
    return event;
}
