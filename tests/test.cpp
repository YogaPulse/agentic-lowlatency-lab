#include <gtest/gtest.h>
#include <order_book.h>
#include <synthetic_feed.h>
#include <instrument_registry.h>

struct OrderBookTestPeer
{
    static std::size_t level_index(const OrderBook& book, OrderId order_id)
    {
        return book._orders.at(order_id).level_index;
    }
};

namespace
{
MarketDataEvent event(
    Action action,
    Side side,
    std::int64_t price_ticks,
    std::uint64_t quantity,
    std::uint32_t instrument_id = 1,
    OrderId order_id = 1)
{
    return MarketDataEvent{
        .order_id = order_id,
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
    ASSERT_NE(event1.order_id, 0);

    ASSERT_TRUE(event2.timestamp_ns > event1.timestamp_ns);
}

TEST(SyntheticFeedTest, SameSeedProducesSameEvents) {
    SyntheticFeed first_feed{42};
    SyntheticFeed second_feed{42};

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(first_feed.next().to_str(), second_feed.next().to_str());
    }
}

TEST(SyntheticFeedTest, ProducesValidOrderLifecycles) {
    SyntheticFeed feed{42};
    OrderBook book{1, 1'024};

    for (int i = 0; i < 10'000; ++i)
    {
        EXPECT_EQ(book.apply(feed.next()), ApplyResult::Applied);
    }
}

TEST(MarketDataEventTest, FormatsOrderId) {
    const auto market_event = event(Action::Add, Side::Buy, 100, 10, 1, 42);

    EXPECT_NE(market_event.to_str().find("order_id 42"), std::string::npos);
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

TEST(OrderBookTest, CachesNonzeroLevelIndexOnAdd) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 10, 1, 1)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 20, 1, 2)), ApplyResult::Applied);

    ASSERT_EQ(book.level_count(Side::Buy), 2);
    EXPECT_EQ(OrderBookTestPeer::level_index(book, 2), 1);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 20);
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

TEST(OrderBookTest, LazilyRepairsStaleLevelIndexHint) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10, 1, 1)), ApplyResult::Applied);

    // Inserting a better bid shifts the existing level without eagerly updating
    // the order's cached index.
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 20, 1, 2)), ApplyResult::Applied);
    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_EQ(book.best_bid()->price_ticks, 101);
    ASSERT_EQ(OrderBookTestPeer::level_index(book, 1), 0);

    // The stale hint falls back to price lookup and is repaired to index 1.
    ASSERT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 15, 1, 1)), ApplyResult::Applied);
    ASSERT_EQ(OrderBookTestPeer::level_index(book, 1), 1);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 15);

    // A subsequent update uses the repaired hint and preserves the aggregate.
    ASSERT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 25, 1, 1)), ApplyResult::Applied);
    EXPECT_EQ(OrderBookTestPeer::level_index(book, 1), 1);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 25);

    // Removing the preceding level makes the repaired hint out of bounds; the
    // same fallback repairs that form of staleness as well.
    ASSERT_EQ(book.apply(event(Action::Delete, Side::Buy, 101, 0, 1, 2)), ApplyResult::Applied);
    ASSERT_EQ(OrderBookTestPeer::level_index(book, 1), 1);
    ASSERT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 30, 1, 1)), ApplyResult::Applied);
    EXPECT_EQ(OrderBookTestPeer::level_index(book, 1), 0);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 30);
}

TEST(OrderBookTest, LazilyRepairsStaleAskLevelIndexHint) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 20, 1, 1)), ApplyResult::Applied);

    // Inserting a better ask shifts the existing level and makes its cached
    // index stale.
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 101, 10, 1, 2)), ApplyResult::Applied);
    ASSERT_EQ(OrderBookTestPeer::level_index(book, 1), 0);

    // The first update falls back to lookup and repairs the ask-side hint.
    ASSERT_EQ(book.apply(event(Action::Update, Side::Sell, 102, 25, 1, 1)), ApplyResult::Applied);
    ASSERT_EQ(OrderBookTestPeer::level_index(book, 1), 1);
    EXPECT_EQ(book.quantity_at(Side::Sell, 102), 25);

    // The repaired index remains valid for the subsequent update.
    ASSERT_EQ(book.apply(event(Action::Update, Side::Sell, 102, 35, 1, 1)), ApplyResult::Applied);
    EXPECT_EQ(OrderBookTestPeer::level_index(book, 1), 1);
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
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 20, 1, 2)), ApplyResult::Applied);

    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(book.best_bid()->price_ticks, 101);
    EXPECT_EQ(book.best_bid()->quantity, 20);
}

TEST(OrderBookTest, BestAsk) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 103, 30)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 40, 1, 2)), ApplyResult::Applied);

    ASSERT_TRUE(book.best_ask().has_value());
    EXPECT_EQ(book.best_ask()->price_ticks, 102);
    EXPECT_EQ(book.best_ask()->quantity, 40);
}

TEST(OrderBookTest, MultiplePriceLevels) {
    OrderBook book{1};

    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 99, 10, 1, 1)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 20, 1, 2)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 30, 1, 3)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 102, 40, 1, 4)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 103, 50, 1, 5)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 104, 60, 1, 6)), ApplyResult::Applied);

    EXPECT_EQ(book.level_count(Side::Buy), 3);
    EXPECT_EQ(book.level_count(Side::Sell), 3);
    EXPECT_EQ(book.quantity_at(Side::Buy, 99), 10);
    EXPECT_EQ(book.quantity_at(Side::Sell, 104), 60);
}

TEST(OrderBookTest, RejectsInvalidEventsWithoutMutation) {
    OrderBook book{1};

    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 0)), ApplyResult::InvalidQuantity);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10, 1, 0)), ApplyResult::InvalidOrderId);
    EXPECT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 10)), ApplyResult::OrderNotFound);
    EXPECT_EQ(book.apply(event(Action::Delete, Side::Buy, 100, 0)), ApplyResult::OrderNotFound);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10, 2)), ApplyResult::InstrumentMismatch);

    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);
    EXPECT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 20)), ApplyResult::OrderAlreadyExists);
    EXPECT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 0)), ApplyResult::InvalidQuantity);
    EXPECT_EQ(book.apply(event(Action::Delete, Side::Buy, 100, 1)), ApplyResult::InvalidQuantity);
    EXPECT_EQ(book.apply(event(Action::Update, Side::Sell, 100, 20)), ApplyResult::OrderAttributesMismatch);
    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 10);
}

TEST(OrderBookTest, ChangesBestPriceAfterDelete) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 101, 20, 1, 2)), ApplyResult::Applied);

    ASSERT_EQ(book.apply(event(Action::Delete, Side::Buy, 101, 0, 1, 2)), ApplyResult::Applied);
    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(book.best_bid()->price_ticks, 100);
}

TEST(OrderBookTest, AggregatesOrdersAtSamePrice) {
    OrderBook book{1};

    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10, 1, 1)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 20, 1, 2)), ApplyResult::Applied);

    EXPECT_EQ(book.quantity_at(Side::Buy, 100), 30);
    EXPECT_EQ(book.order_count_at(Side::Buy, 100), 2);
    EXPECT_EQ(book.order_count(), 2);
    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(book.best_bid()->order_count, 2);
}

TEST(OrderBookTest, PreservesFifoPriority) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 10, 1, 1)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 20, 1, 2)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Buy, 100, 30, 1, 3)), ApplyResult::Applied);

    ASSERT_TRUE(book.first_order_at(Side::Buy, 100).has_value());
    EXPECT_EQ(book.first_order_at(Side::Buy, 100)->order_id, 1);
    ASSERT_EQ(book.apply(event(Action::Update, Side::Buy, 100, 15, 1, 1)), ApplyResult::Applied);
    EXPECT_EQ(book.first_order_at(Side::Buy, 100)->order_id, 1);
    ASSERT_EQ(book.apply(event(Action::Delete, Side::Buy, 100, 0, 1, 1)), ApplyResult::Applied);
    EXPECT_EQ(book.first_order_at(Side::Buy, 100)->order_id, 2);
}

TEST(OrderBookTest, DeletesMiddleOrderAndMaintainsAggregate) {
    OrderBook book{1};
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 100, 10, 1, 1)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 100, 20, 1, 2)), ApplyResult::Applied);
    ASSERT_EQ(book.apply(event(Action::Add, Side::Sell, 100, 30, 1, 3)), ApplyResult::Applied);

    ASSERT_EQ(book.apply(event(Action::Delete, Side::Sell, 100, 0, 1, 2)), ApplyResult::Applied);
    EXPECT_FALSE(book.order(2).has_value());
    EXPECT_EQ(book.quantity_at(Side::Sell, 100), 40);
    EXPECT_EQ(book.order_count_at(Side::Sell, 100), 2);
    EXPECT_EQ(book.first_order_at(Side::Sell, 100)->order_id, 1);

    ASSERT_EQ(book.apply(event(Action::Delete, Side::Sell, 100, 0, 1, 1)), ApplyResult::Applied);
    ASSERT_TRUE(book.first_order_at(Side::Sell, 100).has_value());
    EXPECT_EQ(book.first_order_at(Side::Sell, 100)->order_id, 3);
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
