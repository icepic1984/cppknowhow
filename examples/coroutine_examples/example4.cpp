// Pass data back and forth between coroutine

#include <coroutine>
#include <iostream>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

template <typename PromiseType = void>
struct suspend_always
{
    PromiseType* m_promise;

    bool await_ready() const noexcept
    {
        DBG;
        return false;
    }
    void await_suspend(std::coroutine_handle<PromiseType> h) noexcept
    {
        DBG;
        m_promise = &h.promise();
    }
    PromiseType* await_resume() const noexcept
    {
        DBG;
        return m_promise;
    }
};

struct ReturnObject
{
    struct promise_type
    {
        int m_value = 12;

        promise_type()
        {
            DBG;
        }
        ReturnObject get_return_object()
        {
            DBG;
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend()
        {
            DBG;
            return {};
        }

        std::suspend_never final_suspend() noexcept
        {
            DBG;
            return {};
        }

        void unhandled_exception()
        {
        }

        int getValue() const noexcept
        {
            return m_value;
        }
        void setValue(int value) noexcept
        {
            m_value = value;
        }
    };
    ReturnObject(std::coroutine_handle<promise_type> handle)
        : m_handle(handle){};

    std::coroutine_handle<promise_type> handle()
    {
        return m_handle;
    }

private:
    std::coroutine_handle<promise_type> m_handle;
};

ReturnObject test()
{
    DBG;
    std::cout << "First suspend" << std::endl;
    auto promise = co_await suspend_always<ReturnObject::promise_type>{};
    std::cout << "Resume with: " << promise->getValue() << std::endl;
    co_await std::suspend_always{};
    std::cout << "Resume with: " << promise->getValue() << std::endl;
}

int main()
{
    DBG;
    std::cout << "Call test" << std::endl;
    auto bla = test();
    std::cout << "Calling Resume 1" << std::endl;
    bla.handle().resume();
    std::cout << "Set new value" << std::endl;
    bla.handle().promise().setValue(10);
    std::cout << "Calling Resume 2" << std::endl;
    bla.handle().resume();
    std::cout << "done" << std::endl;
}
