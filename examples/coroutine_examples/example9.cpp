#include <coroutine>
#include <iostream>
#include <utility>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

using namespace std;

class task
{
public:
    inline static int counter = 0;

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
        std::cout << "Destroy Task-Object nr: " << counter-- << std::endl;
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
        std::cout << "Create Task-Object nr: " << ++counter << std::endl;
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

    // What happens?
    // ** bar **
    // 0) Call `bar()`
    //
    // 1) Allocate a coroutine frame
    //
    // 2) Copy parameters and construct promise object `promise_type`
    //
    // 3) Call ~promise.get_return_object()~ which constructs ~task~
    // object with the current coroutine handle and saves the result
    // in a local variable.
    //
    // 4) `co_await` `promise.initial_suspend()` which evaluates to
    // `suspend_always` and suspend the coroutine and return the task
    // object to the caller.
    //
    // *** start ***
    //
    // 5) Call `start()` which in turn is a coroutine
    //
    // 6) Allocate a coroutine frame, copy parameters and construct
    // promise type.
    //
    // 7) Call ~promise.get_return_object()~ which constructs
    // ~sync_wait_task~ with the current coroutine handle and saves
    // result in a local variable.
    //
    // 8) `co_await` `promise.initial_suspend()` which evaluates to
    // `suspend_never` and resumes execution.
    //
    // 9) Body of `start()` => `co_await task` is executed which
    // evaluates to the construction of `awaiter`
    //
    // 10) Exectue `await_ready` and `await_suspend` of `awaiter`
    // which stores current continuation in coroutine handle of
    // promise and resumes coroutine of task, which is currently
    // suspended at the initial suspend point of `bar`.
    //
    // *** bar ***
    //
    // 11) Body of `bar()` is executed
    //
    // ** foo **
    //
    // 12) Call `foo()` which is a coroutine
    //
    // 13) Allocate a coroutine frame, copy parameters and construct
    // promise type.
    //
    // 14) Call ~promise.get_return_object()~ which constructs
    // ~task~ with the current coroutine handle and saves
    // result in a local variable.
    //
    // 15) `co_await` `promise.initial_suspend()` which evaluates to
    // `suspend_always` and suspend coroutine. Return constructed task
    // object to caller, namely `bar()`
    //
    // ** bar **
    //
    // 16) Exectue `await_ready` and `await_suspend` of `awaiter`
    // which stores current continuation in coroutine handle of
    // promise and resumes coroutine of task, which is currently
    // suspended at the initial suspend point of `foo`.
    //
    // ** foo **
    //
    // 17) Call `promise_type::return_void` because `co_return` is
    // reached.
    //
    // 18) Call `promise_type::final_suspend` which co_awaits on a
    // `final_awaiter` i.e. call `await_ready` and `await_suspend`
    // which will suspend the current coroutine but resumes the
    // continuation which was stored in the task object priviously.
    //
    // 19) Call task::awaiter::await_resume() and resume in bar()
    //
    // **bar**
    //
    // 20) The bar() coroutine resumes and continues executing and
    // eventually reaches the end of the statement containing the
    // co_await expression at which point it calls the destructor of
    // the temporary task object returned from foo(). The task
    // destructor then calls the .destroy() method on foo()’s
    // coroutine handle which then destroys the coroutine frame along
    // with the promise object and copies of any arguments.
    //
    // 21) Coroutine **bar** drops of the end and calls `return_void`.
    //
    // 22) co_awaits the `final_suspend()` which returns the `final_awaiter`.
    //
    // 23) `await_ready()` and `await_suspend` is called which will
    // suspend and resumes execution at the continuation.
    //
    // **sync_wait_task**
    //
    // 24) Resumes at `sync_wait_task` `co_await std;:move(t)` which
    // will eventually reach the end and call the task destructor.

    DBG;
    // manual_executor ex;
    auto b = sync_wait_task::start(bar());
    // b.resume();
}
