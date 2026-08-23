#pragma once

#include <cstdint>
#include <format>
#include <string>

using OrderId = std::uint64_t;

enum class Side : uint8_t
{
    Buy,
    Sell
};

enum class Action : uint8_t
{
    Add,
    Update,
    Delete
};

struct MarketDataEvent
{
    uint64_t timestamp_ns{};
    OrderId order_id{};
    uint32_t instrument_id{};
    int64_t price_ticks{};
    uint64_t quantity{};
    Side side{};
    Action action{};

    std::string to_str() const {
        return std::format("MarketDataEvent: ts {}, order_id {}, instrument_id {}, price_ticks {}, qty {}, side {}, action {}",
                           timestamp_ns, order_id, instrument_id, price_ticks, quantity, static_cast<int>(side), static_cast<int>(action));
    }
};
