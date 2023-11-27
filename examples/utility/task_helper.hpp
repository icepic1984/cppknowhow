#include "trace.hpp"

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

template <typename T>
struct ResultOfWaitImpl
{
    using value_type = std::remove_reference_t<decltype(
        std::declval<T>().operator co_await().await_resume())>;
};

template <typename T>
using ResultOfWait = typename ResultOfWaitImpl<T>::value_type;

template <typename T>
SyncWaitImpl<typename ResultOfWaitImpl<T>::value_type> syncWaitImpl(T&& task)
{
    DBG;
    co_return co_await std::forward<T>(task);
}

template <typename T>
auto syncWait(T&& task)
{
    DBG;
    return syncWaitImpl(std::forward<T>(task)).result.get();
}

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
            std::this_thread::sleep_for(std::chrono::seconds(5));
            result.assign(std::istreambuf_iterator<char>{stream},
                std::istreambuf_iterator<char>{});
            std::cout << std::this_thread::get_id()
                      << " worker thread: resuming coro: " << coro.address()
                      << "\n";
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

auto SwitchToNewThread(std::jthread& out)
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
