#include "synchronized_value.hpp"
#include <iostream>
#include <map>
#include <queue>
#include <optional>

// Examples and main function

// Simple read/write operations
void example_const_usage() {
    std::cout << "\n=== Basic Usage Example ===\n";

    BM::synchronized_value<std::string> s("Hello");

    // Read value
    std::string read_value = BM::apply(
        [](const auto& x) { return x; }, s);
    std::cout << "Initial value: " << read_value << std::endl;

    // Write value - note: this should use non-const lambda parameter
    BM::apply([](auto& x) { x = "World"; }, s);

    // Read the updated value
    std::string updated_value = BM::apply(
        [](const auto& x) { return x; }, s);
    std::cout << "Updated value: " << updated_value << std::endl;
}

// Example demonstrating mutable member usage in const methods
class ThreadSafeCache {
private:
    BM::synchronized_value<std::map<std::string, int>> cache;

public:
    ThreadSafeCache() = default;

    // This const method can now access the mutable synchronized_value member!
    int get_value(const std::string& key) const {
        return BM::apply([&](const auto& cache_map) -> int {
            auto it = cache_map.find(key);
            return (it != cache_map.end()) ? it->second : -1;
        }, cache);
    }

    // Non-const method can modify normally
    void set_value(const std::string& key, int value) {
        BM::apply([&](auto& cache_map) {
            cache_map[key] = value;
        }, cache);
    }

    // Const method that demonstrates read-only access
    bool contains_key(const std::string& key) const {
        return BM::apply([&](const auto& cache_map) {
            return cache_map.find(key) != cache_map.end();
        }, cache);
    }

    size_t size() const {
        return BM::apply([](const auto& cache_map) {
            return cache_map.size();
        }, cache);
    }
};

// Example demonstrating the fixed mutable member issue
void example_mutable_member_fixed() {
    std::cout << "\n=== Mutable Member Fix Example ===\n";

    ThreadSafeCache cache;

    // Set some values (non-const operations)
    cache.set_value("key1", 42);
    cache.set_value("key2", 84);

    // Create a const reference to test const methods
    const ThreadSafeCache& const_cache = cache;

    // This now works! const method accessing mutable synchronized_value member
    int value1 = const_cache.get_value("key1");
    int value2 = const_cache.get_value("key2");
    int missing = const_cache.get_value("missing_key");
    bool has_key1 = const_cache.contains_key("key1");
    bool has_missing = const_cache.contains_key("missing_key");
    size_t cache_size = const_cache.size();

    std::cout << "Value for 'key1': " << value1 << std::endl;
    std::cout << "Value for 'key2': " << value2 << std::endl;
    std::cout << "Value for 'missing_key': " << missing << std::endl;
    std::cout << "Has 'key1': " << (has_key1 ? "yes" : "no") << std::endl;
    std::cout << "Has 'missing_key': " << (has_missing ? "yes" : "no") << std::endl;
    std::cout << "Cache size: " << cache_size << std::endl;
}

int main() {
    std::cout << "=== synchronized_value Implementation Test ===\n";
    std::cout << "Based on P0290R4 proposal with mutable member fix\n";

    try {
        example_const_usage();
        example_mutable_member_fixed();

        std::cout << "\n=== All tests completed successfully! ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

