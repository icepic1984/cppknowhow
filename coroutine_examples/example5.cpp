// Resume coroutine in a new thread
// https://devblogs.microsoft.com/oldnewthing/20191209-00/?p=103195
#include <coroutine>
#include <iostream>
#include <thread>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

auto switch_to_new_thread(std::jthread& out)
{
    DBG;
    struct awaitable
    {
        std::jthread* p_out;
        bool await_ready()
        {
            DBG;
            return false;
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            DBG;
            std::jthread& out = *p_out;
            if (out.joinable())
                throw std::runtime_error("Output jthread parameter not empty");
            out = std::jthread([h] { h.resume(); });
            // Potential undefined behavior: accessing potentially destroyed
            // *this std::cout << "New thread ID: " << p_out->get_id() << '\n';
            std::cout << "New thread ID: " << out.get_id()
                      << '\n'; // this is OK
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
    return awaitable{&out};
}

struct task
{
    struct promise_type
    {
        task get_return_object()
        {
            DBG;
            return {};
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
        void return_void()
        {
            DBG;
        }
        void unhandled_exception()
        {
            DBG;
        }
    };
};

task resuming_on_new_thread(std::jthread& out)
{
    DBG;
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id()
              << '\n';
    co_await switch_to_new_thread(out);
    // awaiter destroyed here
    std::cout << "Coroutine resumed on thread: " << std::this_thread::get_id()
              << '\n';
}

int main()
{
    std::jthread out;
    resuming_on_new_thread(out);
}
