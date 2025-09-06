#include "synchronized_value.hpp"
#include <iostream>
#include <map>
#include <queue>
#include <optional>

// Examples and main function

// Simple read/write operations
void example_basic_usage() {
    std::cout << "\n=== Basic Usage Example ===\n";

    BM::synchronized_value<std::string> s("Hello");

    // Read value
    std::string read_value = BM::apply(
        [](auto& x) { return x; }, s);
    std::cout << "Initial value: " << read_value << std::endl;

    // Write value
    BM::apply([](auto& x) { x = "World"; }, s);

    // Read the updated value
    std::string updated_value = BM::apply(
        [](auto& x) { return x; }, s);
    std::cout << "Updated value: " << updated_value << std::endl;
}

// More complex example with queue
void example_queue_processing() {
    std::cout << "\n=== Queue Processing Example ===\n";

    using message_type = std::string;
    BM::synchronized_value<std::queue<message_type>> queue;

    // Add messages to queue
    BM::apply([](auto& q) {
        q.push("Hello World");
        q.push("Second Message");
    }, queue);

    // Process messages
    for (int i = 0; i < 3; ++i) {
        std::optional<message_type> local_message;
        BM::apply([&](std::queue<message_type>& q) {
            if (!q.empty()) {
                local_message.emplace(std::move(q.front()));
                q.pop();
            }
        }, queue);

        if (local_message) {
            std::cout << "Processed: " << local_message.value() << std::endl;
        } else {
            std::cout << "Queue is empty" << std::endl;
        }
    }
}

// Multi-object synchronization example
struct account {
    int balance = 0;
    void withdraw(int amount) {
        balance -= amount;
        std::cout << "Withdrew " << amount << ", new balance: " << balance << std::endl;
    }
    void deposit(int amount) {
        balance += amount;
        std::cout << "Deposited " << amount << ", new balance: " << balance << std::endl;
    }
    int get_balance() const { return balance; }
};

void transfer_money(
    BM::synchronized_value<account>& from_,
    BM::synchronized_value<account>& to_,
    int amount) {

    BM::apply([=](auto& from, auto& to) {
        std::cout << "Transferring " << amount << " units..." << std::endl;
        from.withdraw(amount);
        to.deposit(amount);
    }, from_, to_);
}

void example_money_transfer() {
    std::cout << "\n=== Money Transfer Example ===\n";

    BM::synchronized_value<account> account1, account2;

    // Initialize accounts
    BM::apply([](auto& acc) {
        acc.balance = 1000;
        std::cout << "Account 1 initialized with balance: " << acc.balance << std::endl;
    }, account1);

    BM::apply([](auto& acc) {
        acc.balance = 500;
        std::cout << "Account 2 initialized with balance: " << acc.balance << std::endl;
    }, account2);

    // Transfer money safely
    transfer_money(account1, account2, 100);

    // Check final balances using const access
    int balance1 = BM::apply([](const auto& acc) { return acc.get_balance(); }, account1);
    int balance2 = BM::apply([](const auto& acc) { return acc.get_balance(); }, account2);

    std::cout << "Final - Account 1: " << balance1 << ", Account 2: " << balance2 << std::endl;
}

int main() {
    std::cout << "=== synchronized_value Implementation Test ===\n";
    std::cout << "Based on pristine P0290R4 proposal\n";

    try {
        example_basic_usage();
        example_queue_processing();
        example_money_transfer();

        std::cout << "\n=== All tests completed successfully! ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

