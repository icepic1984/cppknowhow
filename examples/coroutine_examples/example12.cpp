#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <iostream>
#include <coroutine>
#include <unordered_map>
#include <functional>
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <queue>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <type_traits>
#include <variant>

template <size_t N>
struct str
{
    constexpr str(const char* p) noexcept
    {
        std::copy_n(p, N, cstr);
    }

    friend std::ostream& operator<<(std::ostream& os, const str& s)
    {
        return os << s.cstr;
    }
    char cstr[N];
};

template <size_t N>
str(const char (&)[N]) -> str<N>;

template <str name>
struct test
{
    void bla()
    {
        std::cout << name << std::endl;
    }
};

template <typename T>
struct Generator
{
    struct promise_type;

    Generator(Generator&& other) noexcept;

    Generator& operator=(Generator&& other) noexcept;

    ~Generator();

    auto& operator()() const;

private:
    explicit Generator(promise_type& promise) noexcept;

    std::coroutine_handle<promise_type> handle;
};

struct promise_type
{
    auto get_return_object() noexcept
    {
        return Generator{.handle = *this};
    }

    std::suspend_always initial_suspend() const noexcept
    {
        return {};
    }

    std::suspend_always final_suspend() const noexcept
    {
        return {};
    }

    // co_yield expression is turned into co_await
    // promise.yield_value(expression) by the compiler.
    std::suspend_always yield_value(const T& value) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
    {
        result = value;
        return {};
    }

    void return_void() const noexcept {};

    void unhandled_exception() noexcept(
        std::is_nothrow_copy_constructible_v<std::exception_ptr>);

    T& getValue();

private:
    std::variant<std::monostate, T, std::exception_ptr> result;

}

int main()
{
    test<"bla"> bla;
    bla.bla();
}
