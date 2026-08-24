#pragma once

#include "market_data_event.h"

#include <cstdint>
#include <random>
#include <vector>

class SyntheticFeed
{
public:
    explicit SyntheticFeed(uint64_t seed);

    [[nodiscard]] MarketDataEvent next();

private:
    struct ActiveOrder
    {
        OrderId order_id{};
        std::int64_t price_ticks{};
        std::uint64_t quantity{};
        Side side{};
    };

    std::mt19937_64 _rng;

    std::uniform_int_distribution<std::int64_t> _price_offset_distribution;
    std::uniform_int_distribution<std::uint64_t> _quantity_distribution;
    std::uniform_int_distribution<int> _side_distribution;
    std::discrete_distribution<int> _action_distribution;

    std::uint64_t _timestamp_ns{1'000'000'000ULL};
    OrderId _next_order_id{1};
    std::vector<ActiveOrder> _active_orders;

    static constexpr std::uint32_t InstrumentId = 1;
    static constexpr std::size_t MaxActiveOrders = 1'024;

    // 65000.00, scale = 100
    static constexpr std::int64_t BasePrice = 6'500'000;

    // 0.50 in price units 
    static constexpr std::int64_t TickSize = 50;
};
