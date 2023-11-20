// https://www.youtube.com/watch?v=Npiw4cYElng
#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <iostream>
#include <coroutine>
#include <unordered_map>
#include <functional>
#include <memory>
#include <type_traits>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

template <typename Tp>
struct is_valid_await_suspend_return_type : std::false_type
{
};
template <>
struct is_valid_await_suspend_return_type<bool> : std::true_type
{
};
template <>
struct is_valid_await_suspend_return_type<void> : std::true_type
{
};

template <typename Promise>
struct is_valid_await_suspend_return_type<std::coroutine_handle<Promise>>
    : std::true_type
{
};
template <typename Tp>
concept AwaitSuspendReturnType =
    is_valid_await_suspend_return_type<Tp>::value;

template <typename Tp>
concept Awaiter = requires(Tp&& awaiter, std::coroutine_handle<void> h)
{
    // await_ready() result must be contextually convertible to bool.
    awaiter.await_ready() ? void() : void();
    awaiter.await_suspend(h);
    requires AwaitSuspendReturnType<decltype(awaiter.await_suspend(h))>;
    awaiter.await_resume();
};

struct is_awaiter_test
{
    struct promise_type
    {
        constexpr std::suspend_always initial_suspend() const
        {
            return {};
        }
        constexpr std::suspend_always final_suspend() const noexcept
        {
            return {};
        }
        void unhandled_exception()
        {
        }
        is_awaiter_test get_return_object()
        {
            return {};
        }

        void return_void() noexcept {};
    };
};

template <typename T>
concept is_awaiter = requires()
{
    [](T t) -> is_awaiter_test { co_await t; };
};

template <typename V>
struct value_awaiter
{
    V value;
    constexpr bool await_ready()
    {
        DBG;
        return true;
    }

    void await_suspend(auto)
    {
        DBG;
    }

    V await_resume()
    {
        DBG;
        return std::move(value);
    }
};

struct task
{
    struct promise_type
    {
        std::coroutine_handle<> continuation{std::noop_coroutine()};
        std::exception_ptr error{};

        struct final_awaiter : std::suspend_always
        {
            promise_type* promise;
            std::coroutine_handle<> await_suspend(auto) noexcept
            {
                DBG;
                return promise->continuation;
            }
        };

        std::suspend_always initial_suspend()
        {
            DBG;
            return {};
        }

        final_awaiter final_suspend() noexcept
        {
            DBG;
            return {{}, this};
        }
        task get_return_object()
        {
            DBG;
            return task{unique_promise{this}};
        }

        void unhandled_exception()
        {
            DBG;
            error = std::current_exception();
        }

        void return_void() noexcept
        {
            DBG;
        };

        template <typename A>
        requires Awaiter<A> auto await_transform(A a)
        {
            DBG;
            return a;
        };

        template <typename V>
        requires(!Awaiter<V>) auto await_transform(V v)
        {
            DBG;
            return value_awaiter<V>{std::move(v)};
        }
    };

    using deleter = decltype([](promise_type* p) {
        DBG;
        std::coroutine_handle<promise_type>::from_promise(*p).destroy();
    });

    using unique_promise = std::unique_ptr<promise_type, deleter>;

    unique_promise promise;

    void start()
    {
        DBG;
        auto h = std::coroutine_handle<promise_type>::from_promise(*promise);
        h.resume();
    }

    struct nested_awaiter
    {
        promise_type* promise;
        bool await_ready() const
        {
            DBG;
            return false;
        };
        void await_suspend(std::coroutine_handle<> continuation)
        {
            DBG;
            promise->continuation = continuation;
            std::coroutine_handle<promise_type>::from_promise(*promise)
                .resume();
        };
        void await_resume()
        {
            DBG;
        }
    };
    nested_awaiter operator co_await()
    {
        return nested_awaiter{promise.get()};
    }
};

struct io
{
    std::unordered_map<int, std::function<void(std::string)>> outstanding;
    void submit(int fd, auto fun)
    {
        DBG;
        outstanding[fd] = fun;
    }

    void complete(int fd, std::string value)
    {
        DBG;
        auto it = outstanding.find(fd);
        if (it != outstanding.end())
        {
            auto fun = it->second;
            outstanding.erase(it);
            fun(value);
        }
    }
};

struct async_read
{
    io& context;
    int fd;
    std::string value;
    bool await_ready() const
    {
        DBG;
        return false;
    };

    void await_suspend(std::coroutine_handle<> h)
    {
        DBG;
        context.submit(fd, [this, h](const std::string& line) {
            value = line;
            h.resume();
        });
    }

    std::string await_resume()
    {
        DBG;
        return value;
    }
};

int to_be_made_async()
{
    DBG;
    return 17;
}

task g(io& c)
{
    DBG;
    std::cout << "second=" << co_await async_read(c, 1) << "\n";
    std::cout << "thrid=" << co_await async_read(c, 1) << "\n";
}

task f(io& c)
{
    DBG;
    std::cout << "First=" << co_await async_read{c, 1} << "\n";
    std::cout << "Value=" << co_await to_be_made_async() << "\n";
    // std::cout << "Second=" << co_await async_read{c, 1} << "\n";
    co_await g(c);
}

int main()
{
    std::cout << std::unitbuf << std::boolalpha;
    try
    {
        // std::cout << "Start main\n";
        io context;
        auto t = f(context);
        t.start();

        std::cout << Awaiter<value_awaiter<int>> << std::endl;
        std::cout << Awaiter<async_read> << std::endl;
        std::cout << Awaiter<int> << std::endl;

        std::cout << Awaiter<int> << std::endl;

        context.complete(1, "first line");
        std::cout << "Back in main\n";
        context.complete(1, "second line");
        context.complete(1, "thrid line");
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error: " << ex.what() << "\n";
    }
}
