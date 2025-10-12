#include "synchronized_value.hpp"
#include <string>
#include <iostream>
#include <shared_mutex>

// Bring BM::synchronized_value and BM::apply into current scope
// This works without conflict because the two-overload approach ensures
// that apply() with explicit synchronized_value first parameter
// is distinct from std::apply() which takes a tuple as first parameter
using BM::synchronized_value;
using BM::apply;

// Generic type alias for convenience - users can write SV<T> instead of synchronized_value<T>
template<typename T, typename Mutex = std::mutex>
using SV = synchronized_value<T, Mutex>;

// Convenience alias for shared synchronized_value with std::shared_mutex
template<typename T>
using SSV = synchronized_value<T, std::shared_mutex>;

// Example with single synchronized_value
void example_single_sv() {

    std::cout << "\n=== Single SV Example ===\n";

    // Define a synchronized value without BM:: prefix
    synchronized_value<int> counter(100);

    // Read initial value
    int initial = apply([](const auto& x) { return x; }, counter);
    std::cout << "Initial counter: " << initial << "\n";

    // Increment the counter
    apply([](auto& x) {
        x += 42;
        std::cout << "Incremented counter to: " << x << "\n";
    }, counter);

    // Read final value
    int final = apply([](const auto& x) { return x; }, counter);
    std::cout << "Final counter: " << final << "\n";
}

// Example with two synchronized_values
void example_two_svs() {

    std::cout << "\n=== Two SVs Example ===\n";

    // Define two synchronized values without BM:: prefix
    synchronized_value<int> balance1(1000);
    synchronized_value<int> balance2(500);

    // Read initial values
    int bal1 = apply([](const auto& x) { return x; }, balance1);
    int bal2 = apply([](const auto& x) { return x; }, balance2);
    std::cout << "Initial balances: account1=" << bal1 << ", account2=" << bal2 << "\n";

    // Transfer funds between accounts using both SVs together
    int transfer_amount = 250;
    apply([transfer_amount](auto& from, auto& to) {
        from -= transfer_amount;
        to += transfer_amount;
        std::cout << "Transferred " << transfer_amount << " from account1 to account2\n";
        std::cout << "  account1 balance: " << from << "\n";
        std::cout << "  account2 balance: " << to << "\n";
    }, balance1, balance2);

    // Read final values
    bal1 = apply([](const auto& x) { return x; }, balance1);
    bal2 = apply([](const auto& x) { return x; }, balance2);
    std::cout << "Final balances: account1=" << bal1 << ", account2=" << bal2 << "\n";
}

// Example using SV<T> and SSV<T> type aliases
void example_type_aliases() {

    std::cout << "\n=== Type Aliases (SV and SSV) Example ===\n";

    // SV<T> - basic synchronized_value with std::mutex
    SV<int> counter(100);

    // SSV<T> - synchronized_value with std::shared_mutex
    SSV<std::string> config("production");

    // Test SV with const access
    const auto& const_counter = counter;
    int val = apply([](const auto& x) { return x; }, const_counter);
    std::cout << "SV const read: " << val << "\n";

    // Test SV write
    apply([](auto& x) { x += 50; }, counter);
    val = apply([](const auto& x) { return x; }, counter);
    std::cout << "SV after write: " << val << "\n";

    // Test SSV with shared read access
    auto shared_config = config.share();
    std::string cfg = apply([](const auto& x) { return x; }, shared_config);
    std::cout << "SSV shared read: " << cfg << "\n";

    // Test SSV exclusive write
    apply([](auto& x) { x = "staging"; }, config);
    cfg = apply([](const auto& x) { return x; }, config);
    std::cout << "SSV after exclusive write: " << cfg << "\n";

    // Test SSV const access
    const auto& const_config = config;
    cfg = apply([](const auto& x) { return x; }, const_config);
    std::cout << "SSV const read: " << cfg << "\n";

    std::cout << "All type alias tests (SV/SSV) passed!\n";
}

int main() {
    std::cout << "=== Test using declarations ===\n";
    std::cout << "Using synchronized_value and apply without BM:: prefix\n";

    try {
        example_single_sv();
        example_two_svs();
        example_type_aliases();

        std::cout << "\n=== All tests completed successfully! ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
