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

// Forward declarations for detail namespace functions
namespace detail {
    template<typename F, typename SV0, SynchronisedValueLike... SVs>
    auto apply_impl(F&& f, SV0&& sv0, SVs&&... svs);

    template<typename SV>
    auto get_value_ref(SV&& sv) -> auto&;
}

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
    friend class synchronized_value_lockable_adapter<const shared_synchronized_value &>;

    // Friend declaration for detail namespace helper
    template<typename SV>
    friend auto detail::get_value_ref(SV&& sv) -> auto&;

    // Provide access to the underlying mutex for locking
    Mutex& mut() const { return sync_val_.mut; }

    // Provide const access to the value
    const T& value() const { return sync_val_.value; }

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

    // Friend declarations for detail namespace functions
    template<typename F, typename SV0, SynchronisedValueLike... SVs>
    friend auto detail::apply_impl(F&& f, SV0&& sv0, SVs&&... svs);

    template<typename SV>
    friend auto detail::get_value_ref(SV&& sv) -> auto&;

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

    // Friend declaration for detail namespace implementation
    template<typename F, typename SV0, SynchronisedValueLike... SVs>
    friend auto detail::apply_impl(F&& f, SV0&& sv0, SVs&&... svs);

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

// Detail namespace for internal implementation
namespace detail {
    // Helper to extract value reference from any synchronized value type
    template<typename SV>
    auto get_value_ref(SV&& sv) -> auto& {
        using T = std::remove_cvref_t<SV>;
        if constexpr (is_shared_synchronized_value_v<T>) {
            // shared_synchronized_value - returns const T&
            return sv.value();
        } else {
            // synchronized_value - returns T& or const T&
            return sv.value;
        }
    }

    // Unified apply implementation - handles all synchronized value types
    template<typename F, typename SV0, SynchronisedValueLike... SVs>
    auto apply_impl(F&& f, SV0&& sv0, SVs&&... svs)
    {
        // Create lockable adapters for all parameters
        auto adapters = std::tuple{
            synchronized_value_lockable_adapter(std::forward<SV0>(sv0)),
            synchronized_value_lockable_adapter(std::forward<SVs>(svs))...
        };

        // Lock all mutexes and invoke function
        return std::apply([&](auto&... locks) {
            std::scoped_lock lock(locks...);
            return std::invoke(std::forward<F>(f),
                              get_value_ref(std::forward<SV0>(sv0)),
                              get_value_ref(std::forward<SVs>(svs))...);
        }, adapters);
    }
} // namespace detail

// Public apply overloads - thin wrappers with explicit first parameter types.
// These explicit overloads are necessary to avoid ambiguity with std::apply
// from <tuple>. This not only enables `using BM::apply;` to be sprinkled
// around in client code for convenience, but also paves the way towards moving
// the extended synchronized_value with shared locking support directly into
// std namespace one day. Pristine P0290r4 is free from that challenge because
// it doesn't have shared_synchronized_value type and all parameters are
// explicitly and concretely synchronized_value<T>&.  It is anticipated that
// std::apply from <tuple> would be augmented with additional constraints on
// its tuple parameter in order to avoid this ambiguity, should such enriched
// synchronized_value with shared locking support enter the standard.

// Apply overload 1: First parameter is synchronized_value (lvalue reference)
template<typename F, typename T0, Lockable M0, SynchronisedValueLike... SVs>
auto apply(F&& f, synchronized_value<T0, M0>& sv0, SVs&&... svs)
{
    return detail::apply_impl(std::forward<F>(f), sv0, std::forward<SVs>(svs)...);
}

// Apply overload 2: First parameter is shared_synchronized_value (lvalue reference)
template<typename F, typename T0, SharedLockable M0, SynchronisedValueLike... SVs>
auto apply(F&& f, shared_synchronized_value<T0, M0>& sv0, SVs&&... svs)
{
    return detail::apply_impl(std::forward<F>(f), sv0, std::forward<SVs>(svs)...);
}

// Apply overload 3: First parameter is const synchronized_value (const lvalue reference)
template<typename F, typename T0, Lockable M0, SynchronisedValueLike... SVs>
auto apply(F&& f, const synchronized_value<T0, M0>& sv0, SVs&&... svs)
{
    return detail::apply_impl(std::forward<F>(f), sv0, std::forward<SVs>(svs)...);
}

// Apply overload 4: First parameter is const shared_synchronized_value (const lvalue reference)
template<typename F, typename T0, SharedLockable M0, SynchronisedValueLike... SVs>
auto apply(F&& f, const shared_synchronized_value<T0, M0>& sv0, SVs&&... svs)
{
    return detail::apply_impl(std::forward<F>(f), sv0, std::forward<SVs>(svs)...);
}

} // namespace BM
