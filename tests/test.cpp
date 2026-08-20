#include <gtest/gtest.h>
#include "../src/instrument_registry.h"
#include "../src/order_book.h"
#include "../src/synthetic_feed.h"

namespace
{
MarketDataEvent event(
    Action action,
    Side side,
    std::int64_t price_ticks,
    std::uint64_t quantity,
    std::uint32_t instrument_id = 1)
{
    return MarketDataEvent{
        .instrument_id = instrument_id,
        .price_ticks = price_ticks,
        .quantity = quantity,
        .side = side,
        .action = action
    };
}
}

TEST(SyntheticFeedTest, BasicAssertions) {
    SyntheticFeed feed{42};
    const auto event1 = feed.next();
    const auto event2 = feed.next();

    ASSERT_TRUE(event1.price_ticks > 0);
    ASSERT_TRUE(event1.quantity >= 0);
    ASSERT_TRUE(event1.instrument_id != 0);

    ASSERT_TRUE(event2.timestamp_ns > event1.timestamp_ns);
}

TEST(OrderBookTest, StartsEmpty) {
    const OrderBook book{1};

    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.instrument_id(), 1);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST(OrderBookTest, MaintainsBestBidAndAsk) {
    OrderBook book{1};

    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 20)), ApplyResult::Applied);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Sell, 103, 30)), ApplyResult::Applied);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 40)), ApplyResult::Applied);

    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(book.best_bid()->price_ticks, 101);
    EXPECT_EQ(book.best_bid()->quantity, 20);
    ASSERT_TRUE(book.best_ask().has_value());
    EXPECT_EQ(book.best_ask()->price_ticks, 102);
    EXPECT_EQ(book.best_ask()->quantity, 40);
    EXPECT_EQ(book.level_count(Side::Buy), 2);
    EXPECT_EQ(book.level_count(Side::Sell), 2);
}

TEST(OrderBookTest, UpdatesAndDeletesExistingLevels) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);

    EXPECT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 25)), ApplyResult::Applied);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 25);
    EXPECT_EQ(book.apply(event(Action::Delete, Side::Buy, 100, 0)), ApplyResult::Applied);
    EXPECT_FALSE(book.quantity_at(Side::Buy, 100).has_value());
    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTest, RejectsInvalidEventsWithoutMutation) {
    OrderBook book{1};

    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 0)), ApplyResult::InvalidQuantity);
    EXPECT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 10)), ApplyResult::LevelNotFound);
    EXPECT_EQ(book.apply(event(Action::Delete, Side::Buy, 100, 0)), ApplyResult::LevelNotFound);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10, 2)), ApplyResult::InstrumentMismatch);

    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 20)), ApplyResult::LevelAlreadyExists);
    EXPECT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 0)), ApplyResult::InvalidQuantity);
    EXPECT_EQ(book.apply(event(Action::Delete, Side::Buy, 100, 1)), ApplyResult::InvalidQuantity);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 10);
}

TEST(OrderBookTest, ChangesBestPriceAfterDelete) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 20)), ApplyResult::Applied);

    ASSERT_EQ(book.apply(event(Action::Delete, Side::Buy, 101, 0)), ApplyResult::Applied);
    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(book.best_bid()->price_ticks, 100);
}

TEST(InstrumentRegistryTest, LooksUpNamesAndIds) {
    InstrumentRegistry instruments;

    EXPECT_EQ(
        instruments.register_instrument("SYNTH", 1),
        RegisterInstrumentResult::Registered);
    EXPECT_EQ(instruments.id_for("SYNTH"), 1);
    EXPECT_EQ(instruments.name_for(1), "SYNTH");
    EXPECT_FALSE(instruments.id_for("UNKNOWN").has_value());
    EXPECT_FALSE(instruments.name_for(2).has_value());
}

TEST(InstrumentRegistryTest, RejectsDuplicateNamesAndIds) {
    InstrumentRegistry instruments;
    ASSERT_EQ(
        instruments.register_instrument("SYNTH", 1),
        RegisterInstrumentResult::Registered);

    EXPECT_EQ(
        instruments.register_instrument("SYNTH", 2),
        RegisterInstrumentResult::NameAlreadyExists);
    EXPECT_EQ(
        instruments.register_instrument("OTHER", 1),
        RegisterInstrumentResult::IdAlreadyExists);
}
