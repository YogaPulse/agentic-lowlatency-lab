#include "synthetic_feed.h"

SyntheticFeed::SyntheticFeed(std::uint64_t seed)
    : _rng{seed},
      _price_offset_distribution{-20, 20},
      _quantity_distribution{1, 1000},
      _side_distribution{0, 1},
      _action_distribution{50, 35, 15}
{
}

MarketDataEvent SyntheticFeed::next()
{
    const auto price_offset = _price_offset_distribution(_rng);

    MarketDataEvent event{
        .timestamp_ns = _timestamp_ns,
        .order_id = _next_order_id,
        .instrument_id = InstrumentId,
        .price_ticks = BasePrice + price_offset * TickSize,
        .quantity = _quantity_distribution(_rng),
        .side = _side_distribution(_rng) == 0
            ? Side::Buy
            : Side::Sell,
        .action = static_cast<Action>(
            _action_distribution(_rng)
        )
    };

    _timestamp_ns += 100;
    ++_next_order_id;

    if (event.action == Action::Delete)
    {
        event.quantity = 0;
    }

    return event;
}
