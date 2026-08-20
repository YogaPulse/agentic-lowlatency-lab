#include <iostream>
#include "synthetic_feed.h"

void print_event(const auto& event) {
    std::cout << event.to_str() << std::endl;
}

int main() {
    SyntheticFeed feed{42};

    for (int i = 0; i < 20; ++i)
    {
        const auto event = feed.next();
        print_event(event);
    }
    return 0;
}
