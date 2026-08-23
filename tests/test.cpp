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

TEST(OrderBookTest, EmptyBook) {
    const OrderBook book{1};

    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.instrument_id(), 1);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST(OrderBookTest, AddBid) {
    OrderBook book{1};

    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 10);
    EXPECT_EQ(book.level_count(Side::Buy), 1);
}

TEST(OrderBookTest, AddAsk) {
    OrderBook book{1};

    EXPECT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 20)), ApplyResult::Applied);
    EXPECT_EQ(book.quantity_at(Side::Sell, 102), 20);
    EXPECT_EQ(book.level_count(Side::Sell), 1);
}

TEST(OrderBookTest, UpdateBid) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);

    EXPECT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 25)), ApplyResult::Applied);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 25);
}

TEST(OrderBookTest, UpdateAsk) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 20)), ApplyResult::Applied);

    EXPECT_EQ(book.apply(event(Action::Update, Side::Sell, 102, 35)), ApplyResult::Applied);
    EXPECT_EQ(book.quantity_at(Side::Sell, 102), 35);
}

TEST(OrderBookTest, DeleteBid) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);

    EXPECT_EQ(book.apply(event(Action::Delete, Side::Buy, 100, 0)), ApplyResult::Applied);
    EXPECT_FALSE(book.quantity_at(Side::Buy, 100).has_value());
    EXPECT_EQ(book.level_count(Side::Buy), 0);
}

TEST(OrderBookTest, DeleteAsk) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 20)), ApplyResult::Applied);

    EXPECT_EQ(book.apply(event(Action::Delete, Side::Sell, 102, 0)), ApplyResult::Applied);
    EXPECT_FALSE(book.quantity_at(Side::Sell, 102).has_value());
    EXPECT_EQ(book.level_count(Side::Sell), 0);
}

TEST(OrderBookTest, BestBid) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 20)), ApplyResult::Applied);

    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(book.best_bid()->price_ticks, 101);
    EXPECT_EQ(book.best_bid()->quantity, 20);
}

TEST(OrderBookTest, BestAsk) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 103, 30)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 40)), ApplyResult::Applied);

    ASSERT_TRUE(book.best_ask().has_value());
    EXPECT_EQ(book.best_ask()->price_ticks, 102);
    EXPECT_EQ(book.best_ask()->quantity, 40);
}

TEST(OrderBookTest, MultiplePriceLevels) {
    OrderBook book{1};

    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 99, 10)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 20)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 30)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 40)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 103, 50)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 104, 60)), ApplyResult::Applied);

    EXPECT_EQ(book.level_count(Side::Buy), 3);
    EXPECT_EQ(book.level_count(Side::Sell), 3);
    EXPECT_EQ(book.quantity_at(Side::Buy, 99), 10);
    EXPECT_EQ(book.quantity_at(Side::Sell, 104), 60);
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
