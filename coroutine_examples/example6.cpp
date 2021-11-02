// A lazy generator with coroutine
// https://medium.com/codex/painless-c-coroutines-part-4-69117214bfdc

#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>

template <typename T>
struct ReturnObject
{
    struct promise_type
    {
        T val_;
        ReturnObject get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend()
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        std::suspend_always yield_value(T value)
        {
            val_ = value;
            return {};
        }
        void unhandled_exception()
        {
        }
    };

    std::coroutine_handle<promise_type> h_;
    ReturnObject(std::coroutine_handle<promise_type> h)
        : h_{h}
    {
    }
    operator std::coroutine_handle<promise_type>() const
    {
        return h_;
    }
};

ReturnObject<float> generator()
{
    for (int i = 0;; i++)
    {
        co_yield 1.234 + static_cast<float>(i);
    }
}

using PromType = ReturnObject<float>::promise_type;
int main()
{

    std::coroutine_handle<PromType> h = generator();
    PromType& prom = h.promise();
    for (int i = 0; i < 5; ++i)
    {
        std::cout << "From main: " << prom.val_ << "\n";
        h();
    }
}
