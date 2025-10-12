# std::synchronized_value\<T\>

C++20 implementation of the `synchronized_value` proposal (P0290R4) with **comprehensive extensions**.

Why? Because mutex + data = ❤️!

## P0290R4 Baseline vs Extensions

### ✅ P0290R4 Compliant
- `synchronized_value<T>` template
- Thread-safe `apply()` function with RAII locking  
- Multi-object deadlock-free synchronization

### 🚀 Extensions Beyond P0290R4
- **Custom mutex types** - any `Lockable` type supported, not just `std::mutex`
- **Const support** - synchronized_values work in const methods (because reads need be synchronized too)
- **Shared locking** - `.share()` method for `SharedLockable` mutexes (a.k.a. read-write locks)
- **Mixed exclusive/shared operations** - possible in a single `apply()` call

## Quick Examples

### Basic Usage (P0290R4)
```cpp
BM::synchronized_value<std::string> s("Hello");

// Thread-safe read
auto value = BM::apply([](const auto& x) {
    return x;
}, s);

// Thread-safe write  
BM::apply([](auto& x) {
    x = "World";
}, s);
```

### Custom Mutex Types Extension
```cpp
// Any Lockable mutex type is supported
BM::synchronized_value<std::map<int, int>, std::shared_mutex> data;
BM::apply([](auto& m) { m[1] = 42; }, data);

// Multi-object synchronization with heterogeneous mutex types
BM::synchronized_value<std::string> s("40");
BM::synchronized_value<int, std::shared_mutex> i(2);
auto the_answer = BM::apply([](auto& x, auto& y) {
    return std::stoi(x) + y;
}, s, i);
```

### Const Support Extension  
```cpp
class Cache {
    BM::synchronized_value<std::map<std::string, int>> data;
public:
    // Const method can access mutable synchronized_value member
    int get(const std::string& key) const {
        return BM::apply([&](const auto& m) { 
            return m.count(key) ? m.at(key) : -1; 
        }, data);
    }
};
```

### Shared Locking Extension
```cpp
BM::synchronized_value<std::map<std::string, int>, std::shared_mutex> data;
BM::synchronized_value<std::vector<std::string>, std::shared_mutex> log;

// Exclusive write
BM::apply([](auto& m) {
    m["key"] = 42;
}, data);

// Shared read - multiple concurrent readers can hold the read-lock the same time
BM::apply([](const auto& m) {
    return m.at("key");
}, data.share());

// Mixed exclusive/shared in single operation
BM::apply([](const auto& read_data, auto& write_log) {
    write_log.push_back("Size: " + std::to_string(read_data.size()));
}, data.share(), log);
```

### Using Declarations for Cleaner Code
```cpp
using BM::synchronized_value;
using BM::apply;

// Template alias for convenience
template<typename T, typename Mutex = std::mutex>
using SV = synchronized_value<T, Mutex>;

// Now you can write cleaner code
SV<int> counter(0);
apply([](auto& c) { ++c; }, counter);

// Works seamlessly with custom mutexes
SV<std::string, std::shared_mutex> data("Hello");
apply([](const auto& s) { return s.size(); }, data.share());
```

## Build & Test
```bash
make test        # Build and run examples
```
