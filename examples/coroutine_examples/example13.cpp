// https://www.youtube.com/watch?v=lm10Cj-HNKQ&t=2559s
// https://www.youtube.com/watch?v=7sKUAyWXNHA
// https://github.com/mpusz/mp-coro/blob/main/src/include/mp-coro/task.h

// First part to implement coroutine Implemen promise type which is
// used to communicate with the compiler backend. This is used to
// transfer values from and to the coroutine. The following functions
// must be implemented:
// 1) get_return_object to create: to create a task object from the
// promise type. Is called on entry of the coroutine.
// 2) initial_suspend: specifies the behaviour if the coroutine should
// suspend initially oder continue execution
// 3) return_value: is called on a co_return
// 4) unhandled_exception: is called when an exception in the
// coroutine happens and can be used to store the exception pointer
// 5) final_suspend: specifies the the behaviour on the exit of the coroutine.

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <iostream>
#include <fstream>
#include <coroutine>
#include <unordered_map>
#include <functional>
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <queue>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <type_traits>
#include <variant>
#include <utility>
#include <source_location>
#include <chrono>
#include <filesystem>
#include <future>

auto dbg = [](const char* file, const char* func, std::uint32_t line) {
    std::cerr << "tid: " << std::this_thread::get_id()<< " "<< file << ":" << func << ":" << line << " called.\n";
};

#define DBG                                                                    \
    dbg(std::source_location::current().file_name(),                           \
        std::source_location::current().function_name(),                       \
        std::source_location::current().line())

template <typename T>
struct Promise;

struct CoroDeleter
{
    template <typename Promise>
    void operator()(Promise* promise) const noexcept
    {
        DBG;
        using CoroHandle = std::coroutine_handle<Promise>;
        CoroHandle::from_promise(*promise).destroy();
    }
};

template <typename T>
using PromisePtr = std::unique_ptr<Promise<T>, CoroDeleter>;

template <typename T>
struct [[nodiscard]] Task
{
    using promise_type = Promise<T>;

    using value_type = T;

    Task() = default;

    auto operator co_await() const noexcept
    {
        DBG;
        struct Awaitable
        {
            bool await_ready() const noexcept
            {
                DBG;
                // We ask the promise if the result is ready
                return promise.isReady();
            }

            using CoroHandle = std::coroutine_handle<>;
            // Systemtric control transfer. Suspend continuation
            // passed as parameter to `await_suspend` and resume
            // coroutine returned by `await_suspend` immediately.
            CoroHandle await_suspend(CoroHandle continuation) const noexcept
            {
                DBG;
                promise.continuation = continuation;
                return std::coroutine_handle<Promise<T>>::from_promise(promise);
            }

            T&& await_resume() const
            {
                DBG;
                return promise.getResult();
            }

            Promise<T>& promise;
        };
        return Awaitable{*promise};
    }

private:
    explicit Task(Promise<T>* promise)
        : promise{promise}
    {
        DBG;
    };

    PromisePtr<T> promise = nullptr;

    template <typename>
    friend struct Promise;
};

template <typename T>
struct Promise
{
    std::coroutine_handle<> continuation;

    Task<T> get_return_object() noexcept
    {
        DBG;
        return Task<T>{this};
    }

    // eager execute the task
    std::suspend_never initial_suspend() noexcept
    {
        DBG;
        return {};
    }

    auto final_suspend() noexcept
    {
        struct FinalAwaitable
        {
            bool await_ready() const noexcept
            {
                DBG;
                return false;
            }

            void await_suspend(
                std::coroutine_handle<Promise<T>> thisCoro) noexcept
            {
                DBG;
                auto& promise = thisCoro.promise();
                if (promise.continuation)
                {
                    DBG;
                    std::cerr << "Resume continuation" << std::endl;
                    std::cout << "Resume continuation" << std::endl;
                    //promise.continuation();
                }
            }

            void await_resume() const noexcept
            {
            }
        };
        return FinalAwaitable{};
    }

    // always suspend on final_suspend to retrieve exceptions
    // std::suspend_always final_suspend() noexcept
    // {
    //     DBG;
    //     return {};
    // }

    template <typename U>
    void return_value(U&& value) noexcept(
        std::is_nothrow_constructible_v<T, decltype(std::forward<U>(value))>)
    {
        DBG;
        result.template emplace<1>(std::forward<U>(value));
    }

    void unhandled_exception() noexcept(
        std::is_nothrow_constructible_v<std::exception_ptr, std::exception_ptr>)
    {
        DBG;
        result.template emplace<2>(std::current_exception());
    }

    bool isReady() const noexcept
    {
        DBG;
        return result.index() != 0;
    }

    T&& getResult()
    {
        DBG;
        if (std::holds_alternative<std::exception_ptr>(result))
        {
            std::rethrow_exception(std::get<2>(result));
        }
        return std::move(std::get<1>(result));
    }

    std::variant<std::monostate, T, std::exception_ptr> result;
};

struct AsyncReadFile
{
    AsyncReadFile(std::filesystem::path path)
        : path{std::move(path)}
    {
        DBG;
    }

    bool await_ready() const noexcept
    {
        DBG;
        return false;
    }

    void await_suspend(std::coroutine_handle<> coro)
    {
        DBG;
        auto work = [this, coro]() {
            std::cout << std::this_thread::get_id()
                      << " worker thread: opening file\n";
            auto stream = std::ifstream{path};
            std::cout << std::this_thread::get_id()
                      << " worker thread: reading file\n";
            result.assign(std::istreambuf_iterator<char>{stream},
                std::istreambuf_iterator<char>{});
            std::cout << std::this_thread::get_id()
                      << " worker thread: resuming coro\n";
            coro();
            std::cout << std::this_thread::get_id()
                      << " worker thread: exiting\n";
        };
        std::thread{work}.detach();
    }

    std::string await_resume() noexcept
    {
        DBG;
        return std::move(result);
    }

private:
    std::filesystem::path path;
    std::string result;
};

template <typename T>
struct SyncWaitImpl
{
    struct promise_type
    {
        SyncWaitImpl get_return_object()
        {
            DBG;
            return {promise.get_future()};
        }
        std::suspend_never initial_suspend() noexcept
        {
            DBG;
            return {};
        }

        std::suspend_never final_suspend() noexcept
        {
            DBG;
            return {};
        }

        void return_value(T&& value)
        {
            DBG;
            promise.set_value(std::move(value));
        }

        void unhandled_exception()
        {
            DBG;
            promise.set_exception(std::current_exception());
        }

        std::promise<T> promise;
    };

    std::future<T> result;
};

struct Sleep
{
    bool await_ready() const noexcept
    {
        return duration == duration.zero();
    }

    void await_suspend(std::coroutine_handle<> coro) const
    {
        std::this_thread::sleep_for(duration);
        coro();
    }

    void await_resume() const noexcept
    {
    }

    std::chrono::milliseconds duration;
};

Task<int> foobar()
{
    std::cout << "bla" << std::endl;
    co_await std::suspend_always{};

    co_return 42;
}

Task<int> foo()
{
    DBG;
    std::cout << "foo(): about to return" << std::endl;
    DBG;
    const auto bla = foobar();
    const auto blup = foobar();
    co_await bla;
    co_await blup;
    DBG;
    co_return 10;
}

// Task<int> sleepy()
// {
//     co_await Sleep{std::chrono::seconds{1}};
//     co_return 43;
// }

Task<int> bar()
{
    DBG;
    const auto result = foo();
    DBG;
    std::cout << "bar(): about to co_await\n";
    const int i = co_await result;
    DBG;
    std::cout << "bar(): about to return\n";
    co_return i + 23;
}

Task <int> test2()
{
    std::cout << "First suspend test2" << std::endl;
    co_await std::suspend_always{};
    std::cout << "Second suspend test2" << std::endl;
    //co_await std::suspend_always{};
    co_return 10;
}


Task<int> test()
{
    auto bla = test2();
    std::cout << "First suspend test" << std::endl;
    co_await bla;
    std::cout << "Second suspend test" << std::endl;
    //co_await bla;
    co_return 20;

}

template <typename T>
struct ResultOfWaitImpl
{
    // using type = decltype((std::declval<T>().operator co_wait()).getResult());
    using value_type = typename std::remove_reference<T>::type::value_type;
    
};

template <typename T>
using ResultOfWait = typename ResultOfWaitImpl<T>::value_type;

template<typename T>
SyncWaitImpl<typename ResultOfWaitImpl<T>::value_type> syncWaitImpl(T&& task)
{
    co_return co_await std::forward<T>(task);
}

template <typename T>
auto syncWait(T&& task)
{
    return syncWaitImpl(std::forward<T>(task)).result.get();
}

// SyncWaitImpl<size_t> syncWaitImpl(Task<size_t>& task)
// {
//     DBG;
//     co_return co_await task;
// }

// //template <typename T>
// auto syncWait(Task<size_t>& task)
// {
//     DBG;
//     return syncWaitImpl(task).result.get();
// }


Task<size_t> readFile()
{
    std::cout << std::this_thread::get_id()
              << " readFile(): about to read file async\n";
    const auto result =
        co_await AsyncReadFile{"c:/Projects/git/cppknowhow/type_list.cpp"};
    std::cout << std::this_thread::get_id()
              << " readFile(): about to return (size " << result.size()
              << ")\n";
    co_return result.size();
}
int main()
{
    DBG;
    //auto bla = readFile();
    //syncWait(bla);
    //auto bla = bar();
    auto bla = test();
    DBG;
    //  std::this_thread::sleep_for(std::chrono::seconds(1));
}
