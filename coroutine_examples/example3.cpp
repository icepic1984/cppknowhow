// Add custom awaiteable type

// When a coroutine begins execution, it performs the following:
//
// 1) allocates the coroutine state object using operator new (see
// below)
//
// 2) copies all function parameters to the coroutine state: by-value
// parameters are moved or copied, by-reference parameters remain
// references (and so may become dangling if the coroutine is resumed
// after the lifetime of referred object ends)

// 3) calls the constructor for the promise object. If the promise
// type has a constructor that takes all coroutine parameters, that
// constructor is called, with post-copy coroutine
// arguments. Otherwise the default constructor is called.

// 4) calls promise.get_return_object() and keeps the result in a local
// variable. The result of that call will be returned to the caller
// when the coroutine first suspends. Any exceptions thrown up to and
// including this step propagate back to the caller, not placed in the
// promise.

// 5) calls promise.initial_suspend() and co_awaits its
// result. Typical Promise types either return a suspend_always, for
// lazily-started coroutines, or suspend_never, for eagerly-started
// coroutines.

// 6) when co_await promise.initial_suspend() resumes, starts
// executing the body of the coroutine

// What does `co_await expr` do?:
// 1) expr is converted to an awaitable ...
// 2) the awaiter object is obtained (from the awaitable) ...
// 3) awaiter.await_ready() is called ...
// 4) awaiter.await_suspend(handle) is called ...
// 5) finally, awaiter.await_resume() is called ...

#include <coroutine>
#include <iostream>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

template <typename PromiseType = void>
struct suspend_always
{
    // Compiler checks if the coroutine should be suspended or not by
    // executing the await_ready method. If false is returned, the
    // coroutine should be indeed suspended by calling
    // `await_suspend`. If true is returned, the coroutine should not
    // be suspended. The call to `await_suspend` is skipped.
    //
    // Await_ready is an optimization. Why? If await_ready returns
    // false the method await_suspend is called with the coroutine
    // handle to it and the coroutine state is saved by copying the
    // values from register to coroutine farme, possibly on the
    // heap. The reason this method is provided is incase await_ready
    // returns true and coroutine is not suspended, the cost copying
    // the local variables to the coroutine frame can be avoided and
    // the coroutine state will not be saved.
    bool await_ready() const noexcept
    {
        DBG;
        return true;
    }

    // `await_suspend` can have two return types `bool` and void.
    //
    // 1) void await_suspend(std::coroutine_handle<> h): This type
    // suspends the coroutine.

    // 2) bool await_suspend(std::coroutine_handle<> h): This type
    // allows to conditionally suspend/resume the coroutine. If the
    // value true is, returns control to the caller/resumer of the
    // current coroutine (coroutine is suspended). If the value false,
    // resumes the current coroutine (coroutine is not suspended).
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

        suspend_always<promise_type> final_suspend() noexcept
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
