#include <iostream>
#include "instrument_registry.h"
#include "order_book.h"
#include "synthetic_feed.h"

void print_event(const auto& event) {
    std::cout << event.to_str() << std::endl;
}

int main() {
    InstrumentRegistry instruments;
    const auto registration = instruments.register_instrument("SYNTH", 1);
    if (registration != RegisterInstrumentResult::Registered)
    {
        return 1;
    }

    SyntheticFeed feed{42};
    OrderBook book{*instruments.id_for("SYNTH")};
    std::size_t rejected_events{};

    for (int i = 0; i < 20; ++i)
    {
        const auto event = feed.next();
        print_event(event);
        if (book.apply(event) != ApplyResult::Applied)
        {
            ++rejected_events;
        }
    }

    std::cout << "Accepted events: " << 20 - rejected_events
              << ", rejected events: " << rejected_events << std::endl;
    return 0;
}
