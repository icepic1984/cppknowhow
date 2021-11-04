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

#include <coroutine>
#include <iostream>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

struct ReturnObject
{
    struct promise_type
    {
        promise_type()
        {
            DBG;
        }
        ReturnObject get_return_object()
        {
            DBG;
            return {};
        }

        std::suspend_never initial_suspend()
        {
            DBG;
            return {};
        }

        // std::suspend_always initial_suspend()
        // {
        //     DBG;
        //     return {};
        // }

        // std::suspend_never final_suspend()
        // {
        //     DBG;
        //     return {};
        // }

        std::suspend_never final_suspend() noexcept
        {
            DBG;
            return {};
        }

        void unhandled_exception()
        {
        }
    };
};

ReturnObject test()
{
    DBG;
    std::cout << "intial" << std::endl;
    // Never supsend
    // co_await std::suspend_never{};
    co_await std::suspend_always{};
    std::cout << "Resume" << std::endl;
}

int main()
{
    DBG;
    auto bla = test();
    std::cout << "done" << std::endl;
}
