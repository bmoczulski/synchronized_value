#include "synchronized_value.hpp"
#include <iostream>
#include <map>
#include <shared_mutex>

// Examples showing usage with different mutex types

void example_mutex_variants() {
    std::cout << "\n=== Mutex Variants ===\n";
    // Default (std::mutex)
    BM::synchronized_value<std::string> s("Hello");
    BM::apply([](auto& str){ str += " World"; }, s);

    // Using std::recursive_mutex
    BM::synchronized_value<int, std::recursive_mutex> counter(0);
    BM::apply([](auto& v){ v += 10; }, counter);

    // Using shared mutex is allowed but note: the synchronized_value stores the mutex type.
    // apply() always takes exclusive locks via scoped_lock. If you want shared access semantics
    // you'd need a different helper that acquires shared locks when possible.
    BM::synchronized_value<std::map<int,int>, std::shared_mutex> shared_map;
    BM::apply([](auto& m){ m[1] = 42; }, shared_map);

    // Multiple heterogeneous synchronized_values can be locked together safely
    BM::synchronized_value<int> a(1);
    BM::synchronized_value<int, std::recursive_mutex> b(2);
    BM::apply([](auto& x, auto& y){ x += y; y += x; }, a, b);

    // Print results
    BM::apply([](const auto& str){ std::cout << "s: " << str << "\n"; }, s);
    BM::apply([](const auto& v){ std::cout << "counter: " << v << "\n"; }, counter);
    BM::apply([](const auto& m){ std::cout << "shared_map[1]: " << m.at(1) << "\n"; }, shared_map);
    BM::apply([](const auto& x, const auto& y){ std::cout << "a=" << x << " b=" << y << "\n"; }, a, b);
}



int main() {
    std::cout << "=== synchronized_value Implementation Test ===\n";
    std::cout << "Based on P0290R4 proposal with any Lockable passed as template parameter\n";

    try {
        example_mutex_variants();

        std::cout << "\n=== All tests completed successfully! ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

