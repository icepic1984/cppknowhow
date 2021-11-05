// Wrap async c api with coroutine
// https://devblogs.microsoft.com/oldnewthing/20191209-00/?p=103195
// https://devblogs.microsoft.com/oldnewthing/20191210-00/?p=103197
#include <coroutine>
#include <iostream>
#include <thread>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

std::atomic<bool> finished = false;
void finish(void* handle)
{
    std::cout << "Callback" << std::endl;
    std::coroutine_handle<>::from_address(handle).resume();
}

void someAsyncWork(void (*callback)(void*), void* handle)
{
    std::thread([callback, handle] {
        std::cout << "Start async work" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        callback(handle);
        std::cout << "Done async work" << std::endl;
    })
        .detach();
}

auto asyncCall()
{
    DBG;
    struct awaitable
    {
        bool await_ready()
        {
            DBG;
            return false;
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            DBG;
            someAsyncWork(finish, h.address());
        }
        void await_resume()
        {
            DBG;
        }
        ~awaitable()
        {
            DBG;
        }
    };
    return awaitable{};
}

struct task
{
    task(std::coroutine_handle<> handle)
        : m_handle(handle)
    {
        DBG;
    }

    ~task()
    {
        DBG;
        m_handle.destroy();
    }

    struct promise_type
    {
        task get_return_object()
        {
            DBG;
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend()
        {
            DBG;
            return {};
        }

        // We must suspend here otherwise handle.done() is never true
        std::suspend_always final_suspend() noexcept
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
    };

    std::coroutine_handle<> handle()
    {
        return m_handle;
    }

private:
    std::coroutine_handle<> m_handle;
};

task asyncTask()
{
    DBG;
    co_await asyncCall();
    std::cout << "Coroutine resumed" << '\n';
    finished = true;
}

int main()
{
    auto task = asyncTask();

    while (!task.handle().done())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "Done" << std::endl;
}
