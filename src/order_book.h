#pragma once

#include "market_data_event.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

struct PriceLevel
{
    std::int64_t price_ticks{};
    std::uint64_t quantity{};
    std::size_t order_count{};
};

struct OrderView
{
    OrderId order_id{};
    std::int64_t price_ticks{};
    std::uint64_t quantity{};
    Side side{};
};

enum class ApplyResult : std::uint8_t
{
    Applied,
    InstrumentMismatch,
    InvalidOrderId,
    OrderAlreadyExists,
    OrderNotFound,
    OrderAttributesMismatch,
    InvalidQuantity,
    QuantityOverflow
};

class OrderBook
{
public:
    explicit OrderBook(
        std::uint32_t instrument_id,
        std::size_t expected_order_count = 1'024);

    [[nodiscard]] ApplyResult apply(const MarketDataEvent& event);

    [[nodiscard]] std::uint32_t instrument_id() const noexcept;
    [[nodiscard]] std::optional<PriceLevel> best_bid() const noexcept;
    [[nodiscard]] std::optional<PriceLevel> best_ask() const noexcept;
    // Returns an owned snapshot at zero-based best-to-worst depth, or nullopt
    // when depth is outside the selected side's levels.
    [[nodiscard]] std::optional<PriceLevel> level_at(
        Side side,
        std::size_t depth) const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> quantity_at(
        Side side,
        std::int64_t price_ticks) const noexcept;
    [[nodiscard]] std::optional<OrderView> order(OrderId order_id) const noexcept;
    [[nodiscard]] std::optional<OrderView> first_order_at(
        Side side,
        std::int64_t price_ticks) const noexcept;
    [[nodiscard]] std::size_t order_count() const noexcept;
    [[nodiscard]] std::size_t order_count_at(
        Side side,
        std::int64_t price_ticks) const noexcept;
    [[nodiscard]] std::size_t level_count(Side side) const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    friend struct OrderBookTestPeer;

    struct OrderNode
    {
        std::int64_t price_ticks{};
        std::uint64_t quantity{};
        std::size_t level_index{};
        Side side{};
        OrderId previous{};
        OrderId next{};
    };

    struct LevelState
    {
        std::int64_t price_ticks{};
        std::uint64_t quantity{};
        std::size_t order_count{};
        OrderId first_order_id{};
        OrderId last_order_id{};
    };

    using Levels = std::vector<LevelState>;

    [[nodiscard]] ApplyResult add(const MarketDataEvent& event);
    [[nodiscard]] ApplyResult update(const MarketDataEvent& event);
    [[nodiscard]] ApplyResult remove(const MarketDataEvent& event);

    std::uint32_t _instrument_id;
    Levels _bids;
    Levels _asks;
    std::unordered_map<OrderId, OrderNode> _orders;
};
