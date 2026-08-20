#include <gtest/gtest.h>
#include "../src/synthetic_feed.h"

TEST(SyntheticFeedTest, BasicAssertions) {
    SyntheticFeed feed{42};
    const auto event1 = feed.next();
    const auto event2 = feed.next();

    ASSERT_TRUE(event1.price_ticks > 0);
    ASSERT_TRUE(event1.quantity >= 0);
    ASSERT_TRUE(event1.instrument_id != 0);

    ASSERT_TRUE(event2.timestamp_ns > event1.timestamp_ns);
}
