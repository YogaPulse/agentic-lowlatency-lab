#pragma once

#include "market_data_event.h"

#include <functional>
#include <map>
#include <optional>

struct PriceLevel
{
    std::int64_t price_ticks{};
    std::uint64_t quantity{};
};

enum class ApplyResult : std::uint8_t
{
    Applied,
    InstrumentMismatch,
    LevelAlreadyExists,
    LevelNotFound,
    InvalidQuantity
};

class OrderBook
{
public:
    explicit OrderBook(std::uint32_t instrument_id);

    [[nodiscard]] ApplyResult apply(const MarketDataEvent& event);

    [[nodiscard]] std::uint32_t instrument_id() const noexcept;
    [[nodiscard]] std::optional<PriceLevel> best_bid() const noexcept;
    [[nodiscard]] std::optional<PriceLevel> best_ask() const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> quantity_at(
        Side side,
        std::int64_t price_ticks) const noexcept;
    [[nodiscard]] std::size_t level_count(Side side) const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    using BidLevels = std::map<std::int64_t, std::uint64_t, std::greater<>>;
    using AskLevels = std::map<std::int64_t, std::uint64_t, std::less<>>;

    std::uint32_t _instrument_id;
    BidLevels _bids;
    AskLevels _asks;
};
