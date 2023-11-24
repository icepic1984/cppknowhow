// https://godbolt.org/

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <iostream>
#include <coroutine>
#include <optional>
#include <source_location>
#include <thread>

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

auto dbg = [](const char* file, const char* func, std::uint32_t line) {
    std::cerr << "tid: " << std::this_thread::get_id() << " " << file << ":"
              << func << ":" << line << " called.\n";
};

#define DBG                                                                    \
    dbg(std::source_location::current().file_name(),                           \
        std::source_location::current().function_name(),                       \
        std::source_location::current().line())

// OptionalHolder is returned as proxy class from
// promise->get_return_object.  This struct can be implicitly casted
// to std::optional
template <typename T, typename P>
struct OptionalHolder
{
    std::optional<T> optional;
    OptionalHolder(P* p)
    {
        p->optional = &optional;
    }
    OptionalHolder(OptionalHolder&&) = delete;

    OptionalHolder(OptionalHolder const&) = delete;

    operator std::optional<T>() const noexcept
    {
        std::cout << "doing implicit conversion of result.\n";
        return std::move(optional);
    }
};

template <typename T>
struct OptionalPromise
{
    std::optional<T>* optional = nullptr;

    OptionalHolder<T, OptionalPromise<T>> get_return_object() noexcept
    {
        return {this};
    }

    std::suspend_never initial_suspend() noexcept
    {
        return {};
    }

    std::suspend_never final_suspend() noexcept
    {
        return {};
    }

    void return_value(const T& val) noexcept
    {
        *optional = val;
    }

    void unhandled_exception()
    {
    }

    template <typename U>
    auto await_transform(std::optional<U>& value)
    {
        struct awaiter
        {
            const std::optional<U>* value;

            bool await_ready() noexcept
            {
                return value->has_value();
            }

            void await_suspend(std::coroutine_handle<> handle) noexcept
            {
                handle.destroy();
            }

            U await_resume() noexcept
            {
                return **value;
            };
        };
        return awaiter{&value};
    }
};

namespace std
{
template <typename T, typename... ARGS>
struct coroutine_traits<std::optional<T>, ARGS...>
{
    using promise_type = OptionalPromise<T>;
};
} // namespace std

std::optional<int> test(std::optional<int> a, std::optional<int> b)
{
    co_return(co_await a) * (co_await b);
}

int main()
{
    std::cout << "\n";
    std::optional<int> n = test(10, std::nullopt);
    if (n.has_value())
        std::cout << *n << std::endl;
    else
        std::cout << "has_value == false.\n";

    n = test(10, 20);

    if (n.has_value())
        std::cout << *n << std::endl;
    else
        std::cout << "has_value == false.\n";

    return 0;
}
