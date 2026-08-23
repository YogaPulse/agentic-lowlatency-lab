#include "../src/order_book.h"
#include "../src/synthetic_feed.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr std::size_t EventCount = 1'000'000;

std::string format_count(std::size_t value)
{
    auto result = std::to_string(value);
    for (auto position = result.size(); position > 3; position -= 3)
    {
        result.insert(position - 3, 1, ',');
    }
    return result;
}

std::uint64_t percentile(
    const std::vector<std::uint64_t>& sorted_latencies,
    double percentile_value)
{
    const auto index = static_cast<std::size_t>(
        percentile_value * static_cast<double>(sorted_latencies.size() - 1));
    return sorted_latencies[index];
}
}

int main()
{
    SyntheticFeed feed{42};
    std::vector<MarketDataEvent> events;
    events.reserve(EventCount);
    for (std::size_t i = 0; i < EventCount; ++i)
    {
        events.push_back(feed.next());
    }

    OrderBook book{1};
    std::vector<std::uint64_t> latencies(EventCount);
    std::size_t applied_events{};

    const auto benchmark_start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < EventCount; ++i)
    {
        const auto event_start = std::chrono::steady_clock::now();
        const auto result = book.apply(events[i]);
        const auto event_end = std::chrono::steady_clock::now();

        latencies[i] = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                event_end - event_start).count());
        applied_events += result == ApplyResult::Applied;
    }
    const auto benchmark_end = std::chrono::steady_clock::now();

    const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        benchmark_end - benchmark_start).count();
    const auto average_ns = static_cast<double>(total_ns)
        / static_cast<double>(EventCount);
    const auto events_per_second = static_cast<double>(EventCount) * 1'000'000'000.0
        / static_cast<double>(total_ns);

    std::sort(latencies.begin(), latencies.end());

    std::cout << "Events: " << format_count(EventCount) << '\n'
              << std::fixed << std::setprecision(1)
              << "Throughput: " << events_per_second / 1'000'000.0
              << " M events/sec\n"
              << std::setprecision(0)
              << "Average: " << average_ns << " ns\n"
              << "p50: " << percentile(latencies, 0.50) << " ns\n"
              << "p99: " << percentile(latencies, 0.99) << " ns\n"
              << "p99.9: " << percentile(latencies, 0.999) << " ns\n"
              << "Applied: " << format_count(applied_events) << '\n';
}
