#include <iostream>
#include <variant>
#include <string>
#include <cstdint>
#include <vector>
#include <print>
#include "match.hpp"
using namespace cppmatch;

struct WebEvent
{
    struct PageLoad{};
    struct PageUnload{};
    struct KeyPress{char c;};
    struct Paste{std::string str;};
    struct Click{uint64_t x,y;};
    using webEvent = std::variant<PageLoad, PageUnload, KeyPress, Paste, Click>;
};


int main()
{
    // Create a vector of various web events
    std::vector<WebEvent::webEvent> events = {
        WebEvent::PageLoad{},
        WebEvent::PageUnload{},
        WebEvent::KeyPress{'a'},
        WebEvent::Paste{"Copied text"},
        WebEvent::Click{32, 64},
        WebEvent::KeyPress{'z'},
        WebEvent::Paste{"Another paste"},
        WebEvent::Click{100, 200}
    };

    // Process each event in the vector
    for (const auto& event : events) {
        match(event,
            [](const WebEvent::PageLoad&) {
                std::print("Page loaded\n");
            },
            [](const WebEvent::PageUnload&) {
                std::print("Page unloaded\n");
            },
            [](const WebEvent::KeyPress& kp) {
                std::print("Key pressed: {}\n", kp.c);
            },
            [](const WebEvent::Paste& paste) {
                std::print("Pasted: {}\n", paste.str);
            },
            [](const WebEvent::Click& click) {
                std::print("Clicked at x={}, y={}\n", click.x, click.y);
            }
        );
    }
    
    return 0;
}