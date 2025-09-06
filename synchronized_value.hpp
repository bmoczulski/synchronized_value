#pragma once

#include <type_traits>
#include <mutex>
#include <functional>

#if SV_DEVELOPMENT
#include <iostream>
#endif

namespace BM {

// Concepts
template<typename Mutex>
concept Lockable = requires(Mutex& m) {
    m.lock();
    m.unlock();
    m.try_lock();
    std::is_same_v<decltype(m.lock()), void>;
    std::is_same_v<decltype(m.unlock()), void>;
    std::is_same_v<decltype(m.try_lock()), bool>;
};

template<typename Mutex>
concept SharedLockable = Lockable<Mutex> && requires(Mutex& m) {
    m.lock_shared();
    m.unlock_shared();
    m.try_lock_shared();
    std::is_same_v<decltype(m.lock_shared()), void>;
    std::is_same_v<decltype(m.unlock_shared()), void>;
    std::is_same_v<decltype(m.try_lock_shared()), bool>;
};

// Forward declarations
template<class T, Lockable Mutex>
class synchronized_value;

template<class T, SharedLockable Mutex>
class shared_synchronized_value;

// Helper trait to check if a type is a synchronized_value or shared_synchronized_value
template<typename T>
struct is_synchronized_value : std::false_type {};

template<typename T, typename M>
struct is_synchronized_value<synchronized_value<T, M>> : std::true_type {};

template<typename T, typename M>
struct is_synchronized_value<const synchronized_value<T, M>> : std::true_type {};

template<typename T>
constexpr bool is_synchronized_value_v = is_synchronized_value<std::remove_cvref_t<T>>::value;

template<typename T>
struct is_shared_synchronized_value : std::false_type {};

template<typename T, typename M>
struct is_shared_synchronized_value<shared_synchronized_value<T, M>> : std::true_type {};

template<typename T, typename M>
struct is_shared_synchronized_value<const shared_synchronized_value<T, M>> : std::true_type {};

template<typename T>
constexpr bool is_shared_synchronized_value_v = is_shared_synchronized_value<std::remove_cvref_t<T>>::value;


template<typename T>
constexpr bool is_synchronized_value_like_v = is_synchronized_value_v<T> || is_shared_synchronized_value_v<T>;

template<typename T>
concept SynchronisedValueLike = is_synchronized_value_like_v<T>;

// Helper trait to extract the value type from synchronized_value or shared_synchronized_value
template<typename T, typename = void>
struct extract_value_type;

template<typename T, Lockable M>
struct extract_value_type<synchronized_value<T, M>, void> {
    using type = T&;
};

template<typename T, Lockable M>
struct extract_value_type<const synchronized_value<T, M>, void> {
    using type = const T&;
};

template<typename T, Lockable M>
struct extract_value_type<synchronized_value<T, M>&, void> {
    using type = T&;
};

template<typename T, Lockable M>
struct extract_value_type<const synchronized_value<T, M>&, void> {
    using type = const T&;
};

template<typename T, Lockable M>
struct extract_value_type<synchronized_value<T, M>&&, void> {
    using type = T&;
};

template<typename T, Lockable M>
struct extract_value_type<const synchronized_value<T, M>&&, void> {
    using type = const T&;
};

// Specializations for shared_synchronized_value
template<typename T>
struct extract_value_type<T, std::enable_if_t<is_shared_synchronized_value_v<T>>> {
    using type = const typename T::value_type&;  // shared access is always const
};

template<typename T>
using extract_value_type_t = typename extract_value_type<std::remove_cvref_t<T>>::type;

template<SynchronisedValueLike SyncValue>
class synchronized_value_lockable_adapter;

// Shared synchronized_value class - only available for SharedLockable mutexes
template<class T, SharedLockable Mutex>
class shared_synchronized_value {
public:
    using value_type = T;
    using mutex_type = Mutex;

private:
    synchronized_value<T, Mutex>& sync_val_;

    explicit shared_synchronized_value(synchronized_value<T, Mutex>& sv) : sync_val_(sv) {
#if SV_DEVELOPMENT
        std::cout << "Created shared_synchronized_value<T> from addr = " << &sv << " at addr " << this << "\n";
#endif
    }

    friend class synchronized_value<T, Mutex>;
    friend class synchronized_value_lockable_adapter<shared_synchronized_value>;
    friend class synchronized_value_lockable_adapter<shared_synchronized_value &>;

    // Provide access to the underlying mutex for locking
    Mutex& mut() const { return sync_val_.mut; }

    // Provide const access to the value
    const T& value() const { return sync_val_.value; }

    template<class F, SynchronisedValueLike... SyncValues>
    friend
    auto apply(F&& f, SyncValues&&... values)
        -> std::invoke_result_t<F, extract_value_type_t<SyncValues>...>
        requires (sizeof...(values) != 0);

    // Delete copy/move to avoid confusion
    shared_synchronized_value(const shared_synchronized_value&) = delete;
    shared_synchronized_value& operator=(const shared_synchronized_value&) = delete;
    shared_synchronized_value(shared_synchronized_value&&) = delete;
    shared_synchronized_value& operator=(shared_synchronized_value&&) = delete;
};

// The main synchronized_value class
template<class T, Lockable Mutex = std::mutex>
class synchronized_value {
    using value_type = T;
    using mutex_type = Mutex;

private:
    T value;
    mutable Mutex mut;

    template<class F, SynchronisedValueLike... SyncValues>
    friend
    auto apply(F&& f, SyncValues&&... values)
        -> std::invoke_result_t<F, extract_value_type_t<SyncValues>...>
        requires (sizeof...(values) != 0);

    friend class synchronized_value_lockable_adapter<synchronized_value<T, Mutex> &>;
    friend class synchronized_value_lockable_adapter<const synchronized_value<T, Mutex> &>;


    template<class T2, SharedLockable M2>
    friend class shared_synchronized_value;

public:
    // Delete copy constructor and assignment operator
    synchronized_value(const synchronized_value&) = delete;
    synchronized_value& operator=(const synchronized_value&) = delete;

    // Move constructor and assignment could be implemented but are tricky
    synchronized_value(synchronized_value&&) = delete;
    synchronized_value& operator=(synchronized_value&&) = delete;

    // Constructor template
    template<class... Args>
    synchronized_value(Args&&... args)
        requires (sizeof...(Args) != 1 ||
                 (!std::same_as<synchronized_value, std::remove_cvref_t<Args>> && ...)) &&
                 std::is_constructible_v<T, Args...>
    try : value(std::forward<Args>(args)...) {
#if SV_DEVELOPMENT
        std::cout << "Created synchronized_value<T>\n";
#endif
        // Constructor body - mutex is default constructed
    } catch (...) {
        throw;
    }

    // Shared access member function - only available for SharedLockable mutexes
    template<typename SMutex = Mutex>
        requires SharedLockable<SMutex> && std::same_as<SMutex, Mutex>
    auto share() & -> shared_synchronized_value<T, SMutex>
        requires SharedLockable<SMutex>
    {
        return shared_synchronized_value<T, SMutex>(*this);
    }

    // Prevent share() on temporary objects
    auto share() && = delete;
};

// Deduction guide
template<typename T>
synchronized_value(T) -> synchronized_value<T>;

template<SynchronisedValueLike SyncValue>
class synchronized_value_lockable_adapter {
private:
    std::reference_wrapper<std::remove_reference_t<SyncValue>> sv;

    template<class F, SynchronisedValueLike... SyncValues>
    friend
    auto apply(F&& f, SyncValues&&... values)
        -> std::invoke_result_t<F, extract_value_type_t<SyncValues>...>
        requires (sizeof...(values) != 0);

    synchronized_value_lockable_adapter(SyncValue&& sv) : sv(sv) {
#if SV_DEVELOPMENT
            std::cout << "creating synchronized_value_lockable_adapter\n";
#endif
    }

public:
    void lock() {
        if constexpr (is_shared_synchronized_value_v<SyncValue>) {
            // shared_synchronized_value case - use shared lock
#if SV_DEVELOPMENT
            std::cout << "callling lock_shared()\n";
#endif
            sv.get().mut().lock_shared();
        } else {
            // regular synchronized_value case - use exclusive lock
#if SV_DEVELOPMENT
            std::cout << "callling lock()\n";
#endif
            sv.get().mut.lock();
        }
    }

    void unlock() {
        if constexpr (is_shared_synchronized_value_v<SyncValue>) {
            // shared_synchronized_value case - use shared unlock
#if SV_DEVELOPMENT
            std::cout << "callling unlock_shared()\n";
#endif
            sv.get().mut().unlock_shared();
        } else {
            // regular synchronized_value case - use exclusive unlock
#if SV_DEVELOPMENT
            std::cout << "callling unlock()\n";
#endif
            sv.get().mut.unlock();
        }
    }

    bool try_lock() {
        if constexpr (is_shared_synchronized_value_v<SyncValue>) {
            // shared_synchronized_value case - use shared unlock
#if SV_DEVELOPMENT
            std::cout << "callling try_shared_lock()\n";
#endif
            return sv.get().mut().try_lock_shared();
        } else {
            // regular synchronized_value case - use exclusive unlock
#if SV_DEVELOPMENT
            std::cout << "callling try_lock()\n";
#endif
            return sv.get().mut.try_lock();
        }
    }
};

template<SynchronisedValueLike SyncValue>
synchronized_value_lockable_adapter(SyncValue &&) -> synchronized_value_lockable_adapter<SyncValue>;

// The main apply function - works with both regular and shared synchronized_values
template<class F, SynchronisedValueLike... SyncValues>
auto apply(F&& f, SyncValues&&... values)
    -> std::invoke_result_t<F, extract_value_type_t<SyncValues>...>
    requires (sizeof...(values) != 0)
{
    // Acquire all locks in a deadlock-safe manner
    using LockAdapters = std::tuple<synchronized_value_lockable_adapter<SyncValues>...>;
    LockAdapters lock_adapters(synchronized_value_lockable_adapter(std::forward<SyncValues>(values))...);
    auto lock = std::apply([](auto&... adapters) { return std::scoped_lock(adapters...); }, lock_adapters);

    // Extract the appropriate value references and invoke the function
    auto get_value_ref = [](auto&& sv) -> auto& {
        if constexpr (is_shared_synchronized_value_v<decltype(sv)>) {
            // shared_synchronized_value case - returns const T&
            return sv.value();
        } else {
            // regular synchronized_value case - returns T& or const T&
            return sv.value;
        }
    };

    return std::invoke(std::forward<F>(f), get_value_ref(std::forward<SyncValues>(values))...);
}

} // namespace BM
