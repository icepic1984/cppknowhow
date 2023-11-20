//#https://www.youtube.com/watch?v=8sEe-4tig_A
#include <coroutine>
#include <iostream>
#include <utility>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

struct ReturnType
{
    struct promise_type;

    ReturnType(std::coroutine_handle<promise_type> handle)
        : m_handle(handle)
    {
        DBG;
    }
    struct promise_type
    {
        ReturnType get_return_object()
        {
            DBG;
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend()
        {
            DBG;
            return {};
        }

        void return_void()
        {
            DBG;
        }

        void unhandled_exception()
        {
            DBG;
        }

        std::suspend_always final_suspend() noexcept
        {
            DBG;
            return {};
        }

        std::suspend_always yield_value(int i)
        {
            m_value = i;
            return {};
        }

        int m_value;
    };

    void resume()
    {
        m_handle.resume();
    }

    int value()
    {
        return m_handle.promise().m_value;
    }

    void provide(int value)
    {
        m_handle.promise().m_value = value;
        m_handle.resume();
    }

    void next()
    {
        m_handle.resume();
        
    }

    std::coroutine_handle<promise_type> m_handle;
};

struct Awaitable
{
    bool await_ready()
    {
        DBG;
        return false;
    }

    void await_suspend(std::coroutine_handle<ReturnType::promise_type> h)
    {
        DBG;
    }

    void await_resume()
    {
        DBG;
    }

    int m_value;
};

struct PassOut
{
    PassOut(int value)
        : m_value(value)
    {
    }

    bool await_ready()
    {
        DBG;
        return false;
    }

    void await_suspend(std::coroutine_handle<ReturnType::promise_type> h)
    {
        std::cout << m_value << std::endl;
        DBG;
        h.promise().m_value = m_value;
    }

    void await_resume()
    {
        DBG;
    }

    int m_value;
};

struct PassIn
{
    PassIn()
    {
    }

    bool await_ready()
    {
        DBG;
        return false;
    }

    void await_suspend(std::coroutine_handle<ReturnType::promise_type> h)
    {
        DBG;
        m_handle = h;
    }

    int await_resume()
    {
        DBG;
        return m_handle.promise().m_value;
    }

    std::coroutine_handle<ReturnType::promise_type> m_handle;
};

ReturnType simple()
{
    DBG;
    co_await Awaitable{};
}

ReturnType passOut()
{
    DBG;
    co_yield 10;
}

ReturnType passIn()
{
    DBG;
    int value = co_await PassIn{};
    std::cout << "Value: " << value << std::endl;
}

ReturnType makeFiboGenerator()
{
    int i1 = 1;
    int i2 = 1;
    while (true)
    {
        co_yield i1;
        i1 = std::exchange(i2, i1 + i2);
    }
}

int main()
{
    auto bla = simple();
    bla.resume();

    auto blup = passOut();
    std::cout << "Blup: " << blup.value() << std::endl;

    auto foo = passIn();
    foo.provide(666);

    auto fibo = makeFiboGenerator();
    for(int i = 0; i < 10; fibo.next(), i++)
    {
        std::cout << fibo.value() << std::endl;
    }
}
