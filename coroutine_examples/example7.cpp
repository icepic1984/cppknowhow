// Generator which implements the ranges concept.

#include <coroutine>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>

template <typename T>
struct generator
{
    struct promise_type
    {
        const T* currentValue;

        generator<T> get_return_object()
        {
            return generator<T>{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend()
        {
            return {};
        }

        std::suspend_always final_suspend() noexcept
        {
            return {};
        }

        std::suspend_always yield_value(const T& value)
        {
            currentValue = std::addressof(value);
            return {};
        }

        void unhandled_exception()
        {
        }
    };

    struct iterator
    {
        using iterator_category = std::input_iterator_tag;
        using difference_type = ptrdiff_t;
        using value_type = T;
        using reference = const T&;
        using pointer = const T*;

        std::coroutine_handle<promise_type> coro = nullptr;

        iterator() = default;
        iterator(std::nullptr_t)
            : coro(nullptr)
        {
        }

        iterator(std::coroutine_handle<promise_type> coroArg)
            : coro(coroArg)
        {
        }

        iterator& operator++()
        {
            coro.resume();
            if (coro.done())
                coro = nullptr;
            return *this;
        }

        void operator++(int)
        {
            // This postincrement operator meets the requirements of the Ranges
            // TS InputIterator concept, but not those of Standard C++
            // InputIterator.
            ++*this;
        }

        bool operator==(const iterator& right) const
        {
            return coro == right.coro;
        }

        bool operator!=(const iterator& right) const
        {
            return !(*this == right);
        }

        reference operator*() const
        {
            return *coro.promise().currentValue;
        }

        pointer operator->() const
        {
            return coro.promise().currentValue;
        }
    };

    iterator begin()
    {
        if (coro)
        {
            coro.resume();
            if (coro.done())
                return {nullptr};
        }

        return {coro};
    }

    iterator end()
    {
        return {nullptr};
    }

    explicit generator(std::coroutine_handle<promise_type> handle)
        : coro(handle)
    {
    }

    generator() = default;

    generator(generator const&) = delete;

    generator& operator=(generator const&) = delete;

    generator(generator&& right)
        : coro(right.coro)
    {
        right.coro = nullptr;
    }

    generator& operator=(generator&& right)
    {
        if (this != std::addressof(right))
        {
            coro = right.coro;
            right.coro = nullptr;
        }
        return *this;
    }

    ~generator()
    {
        if (coro)
        {
            coro.destroy();
        }
    }

private:
    std::coroutine_handle<promise_type> coro = nullptr;
};

generator<int> generate(int start, int end)
{
    for (int i = start; i < end; ++i)
    {
        co_yield i;
    }
}

int main()
{
    for (auto i : generate(10, 20) | std::views::drop(2))
    {
        std::cout << i << std::endl;
    }

    std::vector<int> bla{1, 2, 3, 4, 5};

    auto v = bla | std::views::reverse | std::views::drop(2);

    for (auto i : v)
    {
        std::cout << i << std::endl;
    }
}
