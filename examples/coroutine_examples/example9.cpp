#include <coroutine>
#include <iostream>
#include <utility>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

using namespace std;

class task
{
public:
    class promise_type
    {
    public:
        task get_return_object() noexcept
        {
            DBG;
            return task{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_always initial_suspend() noexcept
        {
            DBG;
            return {};
        }

        void return_void() noexcept
        {
            DBG;
        }

        void unhandled_exception() noexcept
        {
            DBG;
            std::terminate();
        }

        struct final_awaiter
        {
            bool await_ready() noexcept
            {
                DBG;
                return false;
            }
            void await_suspend(coroutine_handle<promise_type> h) noexcept
            {
                DBG;
                h.promise().continuation.resume();
            }
            void await_resume() noexcept
            {
                DBG;
            }
        };

        final_awaiter final_suspend() noexcept
        {
            DBG;
            return {};
        }

        coroutine_handle<> continuation;
    };

    task(task&& t) noexcept
        : coro_(std::exchange(t.coro_, {}))
    {
        DBG;
    }

    ~task()
    {
        DBG;
        if (coro_)
            coro_.destroy();
    }

    class awaiter
    {
    public:
        bool await_ready() noexcept
        {
            DBG;
            return false;
        }

        void await_suspend(coroutine_handle<> continuation) noexcept
        {
            DBG;
            // Store the continuation in the task's promise so that the
            // final_suspend() knows to resume this coroutine when the task
            // completes.
            coro_.promise().continuation = continuation;

            // Then we resume the task's coroutine, which is currently suspended
            // at the initial-suspend-point (ie. at the open curly brace).
            coro_.resume();
        }

        void await_resume() noexcept
        {
            DBG;
        }

    private:
        friend task;
        explicit awaiter(coroutine_handle<promise_type> h) noexcept
            : coro_(h)
        {
        }

        coroutine_handle<promise_type> coro_;
    };

    awaiter operator co_await() && noexcept
    {
        DBG;
        return awaiter{coro_};
    }

    void resume()
    {
        coro_.resume();
    }

private:
    explicit task(coroutine_handle<promise_type> h) noexcept
        : coro_(h)
    {
        DBG;
    }

    coroutine_handle<promise_type> coro_;
};

struct sync_wait_task
{
    struct promise_type
    {
        sync_wait_task get_return_object() noexcept
        {
            DBG;
            return sync_wait_task{
                coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_never initial_suspend() noexcept
        {
            DBG;
            return {};
        }

        suspend_always final_suspend() noexcept
        {
            DBG;
            return {};
        }

        void return_void() noexcept
        {
            DBG;
        }

        void unhandled_exception() noexcept
        {
            DBG;
            std::terminate();
        }
    };

    coroutine_handle<promise_type> coro_;

    explicit sync_wait_task(coroutine_handle<promise_type> h) noexcept
        : coro_(h)
    {
        DBG;
    }

    sync_wait_task(sync_wait_task&& t) noexcept
        : coro_(t.coro_)
    {
        DBG;
        t.coro_ = {};
    }

    ~sync_wait_task()
    {
        DBG;
        if (coro_)
        {
            coro_.destroy();
        }
    }

    static sync_wait_task start(task&& t)
    {
        DBG;
        co_await std::move(t);
    }

    bool done()
    {
        DBG;
        return coro_.done();
    }
};

struct manual_executor
{
    struct schedule_op
    {
        manual_executor& executor_;
        schedule_op* next_ = nullptr;
        coroutine_handle<> continuation_;

        schedule_op(manual_executor& executor)
            : executor_(executor)
        {
            DBG;
        }

        bool await_ready() noexcept
        {
            DBG;
            return false;
        }

        void await_suspend(coroutine_handle<> continuation) noexcept
        {
            DBG;
            continuation_ = continuation;
            next_ = executor_.head_;
            executor_.head_ = this;
        }

        void await_resume() noexcept
        {
            DBG;
        }
    };

    schedule_op* head_ = nullptr;

    schedule_op schedule() noexcept
    {
        DBG;
        return schedule_op{*this};
    }

    void drain()
    {
        DBG;
        while (head_ != nullptr)
        {
            auto* item = head_;
            head_ = item->next_;
            item->continuation_.resume();
        }
    }

    void sync_wait(task&& t)
    {
        DBG;
        auto t2 = sync_wait_task::start(std::move(t));
        while (!t2.done())
        {
            drain();
        }
    }
};

task foo()
{
    DBG;
    co_return;
}

task bar()
{
    DBG;
    co_await foo();
}

int main()
{

    // What happens during call of bar?
    // 1) Allocate a coroutine frame
    //
    // 2) Copy parameters and construct promise object `promise_type`
    //
    // 3) Call ~promise.get_return_object()~ which constructs ~task~
    // object with the current coroutine handle and saves the result
    // in a local variable.
    //
    // 4) `co_await` `promise.initial_suspend()` which evaluates to
    // `suspend_always` and suspend the coroutine and return to the
    // caller.

    DBG;
    // manual_executor ex;
    auto b = bar(); //.resume();
    // b.resume();
}
