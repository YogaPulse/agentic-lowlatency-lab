#include <order_book.h>
#include <synthetic_feed.h>
#include <instrument_registry.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

using Clock = std::chrono::steady_clock;

int main(int argc, char** argv) {
    bool json_output = false;

    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--json") {
            json_output = true;
        }
    }

    constexpr std::size_t EventCount = 1'000'000;
    constexpr std::size_t SampleEvery = 100;

    constexpr std::size_t LatencySampleCount =
        (EventCount + SampleEvery - 1) / SampleEvery;

    InstrumentRegistry instruments;
    const auto registration = instruments.register_instrument("SYNTH", 1);
    if (registration != RegisterInstrumentResult::Registered) {
        return 1;
    }

    const auto instrument_id = instruments.id_for("SYNTH");
    if (!instrument_id) {
        return 1;
    }

    std::vector<MarketDataEvent> events;
    events.reserve(EventCount);

    SyntheticFeed feed{42};

    for (std::size_t i = 0; i < EventCount; ++i) {
        events.push_back(feed.next());
    }

    {
        OrderBook warmup_book{*instrument_id};
        constexpr std::size_t WarmupCount = 100'000;

        for (std::size_t i = 0; i < WarmupCount && i < events.size(); ++i) {
            [[maybe_unused]]
            const auto result = warmup_book.apply(events[i]);
        }
    }

    OrderBook throughput_book{*instrument_id};
    std::size_t rejected_events{};

    const auto throughput_start = Clock::now();

    for (const auto& event : events) {
        if (throughput_book.apply(event) != ApplyResult::Applied) {
            ++rejected_events;
        }
    }

    const auto throughput_end = Clock::now();

    const auto total_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            throughput_end - throughput_start
        ).count();

    const double avg_ns =
        static_cast<double>(total_ns) /
        static_cast<double>(events.size());

    const double throughput =
        static_cast<double>(events.size()) *
        1'000'000'000.0 /
        static_cast<double>(total_ns);


    OrderBook latency_book{*instrument_id};
    std::vector<std::uint64_t> latency_samples;
    latency_samples.reserve(LatencySampleCount);

    for (std::size_t i = 0; i < EventCount; ++i)
    {
        if (i % SampleEvery == 0)
        {
            const auto start = Clock::now();

            [[maybe_unused]]
            const auto result = latency_book.apply(events[i]);

            const auto end = Clock::now();

            latency_samples.push_back(
                static_cast<std::uint64_t>(
                    std::chrono::duration_cast<
                        std::chrono::nanoseconds
                    >(end - start).count()
                )
            );
        }
        else
        {
            [[maybe_unused]]
            const auto result = latency_book.apply(events[i]);
        }
    }

    const auto applied_events =
        events.size() - rejected_events;

    const double rejected_percent =
        100.0 *
        static_cast<double>(rejected_events) /
        static_cast<double>(events.size());

    std::sort(latency_samples.begin(), latency_samples.end());

    const auto percentile = [&latency_samples](double p) {
        const auto index = static_cast<std::size_t>(
            p * static_cast<double>(latency_samples.size() - 1));

        return latency_samples[index];
    };

    const auto p50 = percentile(0.50);
    const auto p99 = percentile(0.99);
    const auto p999 = percentile(0.999);

    if (json_output)
    {
        std::cout
            << "{"
            << "\"events\":" << events.size() << ","
            << "\"applied_events\":" << applied_events << ","
            << "\"rejected_events\":" << rejected_events << ","
            << "\"rejected_percent\":" << rejected_percent << ","
            << "\"throughput_events_per_sec\":" << throughput << ","
            << "\"avg_ns\":" << avg_ns << ","
            << "\"p50_ns\":" << p50 << ","
            << "\"p99_ns\":" << p99 << ","
            << "\"p999_ns\":" << p999
            << "}\n";

        return 0;
    }

    std::cout
        << "Events: " << events.size() << '\n'
        << "Applied Events: " << applied_events << '\n'
        << "Rejected Events: " << rejected_events << '\n'
        << "Rejected Percent: " << rejected_percent << '\n'
        << "Throughput: " << throughput << " events/sec\n"
        << "Average: " << avg_ns << " ns/event\n"
        << "p50: " << p50 << " ns\n"
        << "p99: " << p99 << " ns\n"
        << "p99.9: " << p999 << " ns\n";
}
