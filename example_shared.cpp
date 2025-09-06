#include "synchronized_value.hpp"
#include <iostream>
#include <map>
#include <shared_mutex>

// Examples demonstrating the new share() member function

void example_shared_member_function() {
    std::cout << "\n=== Shared Member Function Example ===\n";

    using sync_map_t = BM::synchronized_value<
        std::map<std::string, int>, std::shared_mutex>;

    sync_map_t shared_data;

    // Initialize data with exclusive access
    BM::apply([](auto& data) {
        data["apple"] = 5;
        data["banana"] = 3;
        data["cherry"] = 8;
        std::cout << "Data initialized with " << data.size() << " items\n";
    }, shared_data);

    // Read with shared access using the share() member function
    BM::apply([](const auto& data) {
        auto it = data.find("apple");
        if (it != data.end()) {
            std::cout << "Found apple: " << it->second << " (using shared lock)\n";
        }
        std::cout << "Total items: " << data.size() << "\n";
    }, shared_data.share());

    // Multiple readers can access concurrently
    BM::apply([](const auto& data) {
        std::cout << "Another reader sees " << data.size() << " items\n";
    }, shared_data.share());
}

void example_mixed_exclusive_shared() {
    std::cout << "\n=== Mixed Exclusive and Shared Access ===\n";

    using sync_counter_t = BM::synchronized_value<
        std::map<std::string, int>, std::shared_mutex>;
    using sync_log_t = BM::synchronized_value<
        std::vector<std::string>>;

    sync_counter_t counters;
    sync_log_t event_log;

    // Initialize with exclusive locks
    BM::apply([](auto& c, auto& l) {
        c["requests"] = 0;
        c["errors"] = 0;
        l.push_back("System initialized");
    }, counters, event_log);

    // Increment counter (exclusive) and log event (exclusive)
    BM::apply([](auto& c, auto& l) {
        c["requests"]++;
        l.push_back("Request processed");
        std::cout << "Processed request, total: " << c["requests"] << "\n";
    }, counters, event_log);

    // Read counters (shared) and log (exclusive)
    BM::apply([](const auto& c, auto& l) {
        int total = 0;
        for (const auto& [key, value] : c) {
            total += value;
        }
        l.push_back("Stats check: total=" + std::to_string(total));
        std::cout << "Stats check completed, total events: " << total << "\n";
    }, counters.share(), event_log);

    // Read-only access to both (shared for counters, exclusive for log since it's std::mutex)
    BM::apply([](const auto& c, const auto& l) {
        std::cout << "Summary - Counters: " << c.size() << " entries, Log: " << l.size() << " entries\n";
    }, counters.share(), event_log);
}

void example_backward_compatibility() {
    std::cout << "\n=== Backward Compatibility ===\n";

    // Regular synchronized_value with std::mutex (no share() method)
    BM::synchronized_value<std::string> regular_sync("Hello");

    // This works as before
    BM::apply([](auto& s) {
        s += " World";
        std::cout << "Regular sync value: " << s << "\n";
    }, regular_sync);

    // With shared_mutex, we get the share() method
    BM::synchronized_value<
        std::string, std::shared_mutex> shared_mutex_sync("Shared");

    // Both exclusive and shared access work
    BM::apply([](auto& s) {
        s += " (exclusive)";
    }, shared_mutex_sync);

    BM::apply([](const auto& s) {
        std::cout << "Shared mutex sync value: " << s << "\n";
    }, shared_mutex_sync.share());
}

void example_compile_time_safety() {
    std::cout << "\n=== Compile-time Safety ===\n";

    // This will only compile if Mutex is SharedLockable
    BM::synchronized_value<
        int, std::shared_mutex> shared_lockable(42);

    auto shared_ref = shared_lockable.share(); // OK - std::shared_mutex is SharedLockable

    BM::apply([](const auto& value) {
        std::cout << "Shared access to value: " << value << "\n";
    }, shared_ref);

    // This won't compile - trying to get share() on non-SharedLockable mutex:
    // BM::synchronized_value<int, std::mutex> regular(42);
    // auto bad_ref = regular.share(); // Compilation error!

    std::cout << "Compile-time safety checks passed!\n";
}

void example_two_shared() {
    struct S {
        std::string s;

        S() {
            std::cout << "def-Created S(\"" << s << "\")\n";
        }
        S(const std::string &s) : s(s) {
            std::cout << "Created S(\"" << s << "\") addr = " << this << "\n";
        }
        S(const S& s) {
            std::cout << "Copied S from \"" << s.s << "\"\n";
            this->s = s.s;
        }
        S(S&& s) {
            std::cout << "Moved S (now: \"" << this->s << "\") from \"" << s.s << "\" addr = " << this << "\n";
            this->s = std::move(s.s);
        }
        S& operator=(const S& s) {
            std::cout << "c-assigned S from \"" << s.s << "\"\n";
            this->s = s.s;
            return *this;
        }
        S& operator=(S&& s) {
            std::cout << "m-assigned S (now: \"" << this->s << "\") from \"" << s.s << "\"\n";
            this->s = std::move(s.s);
            return *this;
        }
        ~S() {
            std::cout << "Destroyed S(\"" << s << "\") addr = " << this << "\n";
        }
    };
    std::cout << "\n=== Two shared locks ===\n";

    BM::synchronized_value<
        S, std::shared_mutex> ssv("Shared");

    BM::apply([](auto& s) {
        s.s = "Shared 2";
    }, ssv);

    BM::apply([](const auto& s1, const auto& s2) {
        std::cout << "s1.s = " << s1.s << "\n";
        std::cout << "s2.s = " << s2.s << "\n";
    }, ssv.share(), ssv.share());
}


int main() {
    std::cout << "=== synchronized_value Implementation Test ===\n";
    std::cout << "Based on P0290R4 proposal with shared lock support\n";

    try {
        example_shared_member_function();
        example_mixed_exclusive_shared();
        example_backward_compatibility();
        example_compile_time_safety();
        example_two_shared();

        std::cout << "\n=== All tests completed successfully! ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

