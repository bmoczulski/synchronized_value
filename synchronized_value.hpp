#pragma once

#include <type_traits>
#include <mutex>
#include <functional>

namespace BM {

// Forward declaration
template<class T>
class synchronized_value;

// Helper trait to check if a type is a synchronized_value
template<typename T>
struct is_synchronized_value : std::false_type {};

template<typename T>
struct is_synchronized_value<synchronized_value<T>> : std::true_type {};

template<typename T>
struct is_synchronized_value<const synchronized_value<T>> : std::true_type {};

template<typename T>
constexpr bool is_synchronized_value_v = is_synchronized_value<std::remove_reference_t<T>>::value;

// Helper trait to extract the value type from synchronized_value
template<typename T>
struct extract_value_type;

template<typename T>
struct extract_value_type<synchronized_value<T>&> {
    using type = T&;
};

template<typename T>
struct extract_value_type<const synchronized_value<T>&> {
    using type = const T&;
};

template<typename T>
using extract_value_type_t = typename extract_value_type<T>::type;

template<class T>
class synchronized_value {
    using value_type = T;

private:
    T value;
    mutable std::mutex mut;

    template<class F, class... SyncValues>
    friend
    auto apply(F&& f, SyncValues&... values)
        -> std::invoke_result_t<F, extract_value_type_t<SyncValues&>...>
        requires (sizeof...(values) != 0) &&
                (is_synchronized_value_v<SyncValues> && ...);

public:
    // Delete copy constructor and assignment operator
    synchronized_value(const synchronized_value&) = delete;
    synchronized_value& operator=(const synchronized_value&) = delete;

    // Move constructor and assignment could be implemented but are tricky
    // due to mutex locking requirements, so we'll keep them deleted for simplicity
    synchronized_value(synchronized_value&&) = delete;
    synchronized_value& operator=(synchronized_value&&) = delete;

    // Constructor template
    template<class... Args>
    synchronized_value(Args&&... args)
        requires (sizeof...(Args) != 1 ||
                 (!std::same_as<synchronized_value, std::remove_cvref_t<Args>> && ...)) &&
                 std::is_constructible_v<T, Args...>
    try : value(std::forward<Args>(args)...) {
        // Constructor body - mutex is default constructed
    } catch (...) {
        // Re-throw any exception from T's constructor
        throw;
    }
};

// Deduction guide - needs to be at namespace scope
template<typename T>
synchronized_value(T) -> synchronized_value<T>;


// Apply function - single implementation that handles both const and non-const cases
template<class F, class... SyncValues>
auto apply(F&& f, SyncValues&... values)
    -> std::invoke_result_t<F, extract_value_type_t<SyncValues&>...>
    requires (sizeof...(values) != 0) &&
             (is_synchronized_value_v<SyncValues> && ...)
{
    // Lock all mutexes using std::scoped_lock to avoid deadlocks
    std::scoped_lock lock(values.mut...);

    // Invoke the function with appropriate references (const or non-const based on SyncValues constness)
    return std::invoke(std::forward<F>(f), values.value...);
}

} // namespace BM
