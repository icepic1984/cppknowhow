// Add custom awaiteable type

#include <coroutine>
#include <iostream>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

template <typename PromiseType = void>
struct suspend_always
{
    bool await_ready() const noexcept
    {
        DBG;
        return false;
    }
    void await_suspend(std::coroutine_handle<PromiseType> h) const noexcept
    {
        DBG;
    }
    void await_resume() const noexcept
    {
        DBG;
    }
};

struct ReturnObject
{
    ReturnObject(std::coroutine_handle<> handle)
        : m_handle(handle){};

    struct promise_type
    {
        promise_type()
        {
            DBG;
        }
        ReturnObject get_return_object()
        {
            DBG;
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_always<promise_type> initial_suspend()
        {
            DBG;
            return {};
        }

        suspend_always<promise_type> final_suspend()
        {
            DBG;
            return {};
        }

        void unhandled_exception()
        {
        }
    };

    std::coroutine_handle<> handle()
    {
        return m_handle;
    }

private:
    std::coroutine_handle<> m_handle;
};

ReturnObject test()
{
    DBG;
    std::cout << "intial" << std::endl;
    co_await suspend_always<ReturnObject::promise_type>{};
    std::cout << "Resume" << std::endl;
}

int main()
{
    DBG;
    std::cout << "Call test" << std::endl;
    auto bla = test();
    std::cout << "Calling Resume 1" << std::endl;
    bla.handle().resume();
    std::cout << "Calling Resume 2" << std::endl;
    bla.handle().resume();
    std::cout << "Calling Resume 3" << std::endl;
    bla.handle().resume(); // Segfault
    std::cout << "done" << std::endl;
}
