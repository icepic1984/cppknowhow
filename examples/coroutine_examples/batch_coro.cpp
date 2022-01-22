#include <coroutine>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>
#include <optional>
#include <deque>
#include <memory>
#include <functional>

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

auto dbgr = [](const char* s) {
    std::cout << "Funmction " << s << " resumed.\n";
};

#define DBG dbg(__PRETTY_FUNCTION__)
#define DBGR dbgr(__PRETTY_FUNCTION__)

struct Task
{
    struct promise_type
    {
        Task get_return_object()
        {
            DBG;
            return Task{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend()
        {
            DBG;
            return {};
        }

        void unhandled_exception() noexcept
        {
            DBG;
            std::terminate();
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

        static inline std::size_t placeholder = 0;
        std::size_t* m_counter = &placeholder;
    };

    using Handle = std::coroutine_handle<promise_type>;

    explicit Task(Handle handle)
        : m_handle(handle)
    {
        DBG;
    }

    Task(const Task&) = delete;

    Task(Task&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr))
    {
        DBG;
    }

    ~Task()
    {
        DBG;
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

    Handle release() &&
    {
        DBG;
        return std::exchange(m_handle, nullptr);
    }

    void resume()
    {
        m_handle.resume();
    }

private:
    Handle m_handle;
};

struct Executor
{
    void submit(std::vector<std::coroutine_handle<>> v)
    {
        m_pending.insert(m_pending.end(), v.begin(), v.end());
    }

    void submit(Task t)
    {
        auto handle = std::move(t).release();
        m_pending.push_back(handle);
    }

    std::optional<std::coroutine_handle<>> pop_next_coro()
    {
        if (m_pending.empty())
        {
            return std::nullopt;
        }
        auto first = m_pending.front();
        m_pending.pop_front();
        return first;
    }

    bool run_available()
    {
        for (auto next_coro = pop_next_coro(); next_coro;
             next_coro = pop_next_coro())
        {
            next_coro->resume();
        }
        return true;
    }

private:
    std::deque<std::coroutine_handle<>> m_pending;
};

template <typename T, typename R>
struct Batcher
{
    struct Batch
    {
        std::vector<T> m_args;
        std::vector<R> m_returns;
        std::vector<std::coroutine_handle<>> m_pending;
    };

    struct Awaitable
    {
        bool await_ready()
        {
            DBG;
            return m_storage.maybe_execute();
        }

        R await_resume()
        {
            DBG;
            return m_batch->m_returns.at(m_index);
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
        {
            DBG;
            m_batch->m_pending.push_back(h);
            return m_storage.m_executor.pop_next_coro().value_or(
                std::noop_coroutine());
        }

        Batcher& m_storage;
        std::shared_ptr<Batch> m_batch;
        std::size_t m_index = -1;
    };

    Awaitable operator()(T arg)
    {
        DBG;
        std::size_t index = m_current_batch->m_args.size();
        m_current_batch->m_args.push_back(arg);
        return Awaitable{*this, m_current_batch, index};
    }

    // Return true if the execution resulted in some tasks being unblocked
    bool maybe_execute(bool force = false)
    {
        DBG;
        bool should_execute =
            m_should_execute_op && m_should_execute_op(m_current_batch->m_args);
        if (force || should_execute)
        {
            if (m_current_batch->m_args.empty())
            {
                return false;
            }
            m_current_batch->m_returns = m_op(std::move(m_current_batch->m_args));
            m_executor.submit(std::move(m_current_batch->m_pending));
            m_current_batch->m_pending.clear();
            m_current_batch = std::make_shared<Batch>();
            return true;
        }
        return false;
    }

    Executor& m_executor;
    std::function<std::vector<R>(std::vector<T>)> m_op;
    std::function<bool(const std::vector<T>&)> m_should_execute_op;
    std::shared_ptr<Batch> m_current_batch = std::make_shared<Batch>();
};

Task test1()
{
    DBGR;
    std::cout << "Test 1" << std::endl;
    co_return;
}

Task test2()
{
    DBGR;
    std::cout << "Test 2" << std::endl;
    co_return;
}

std::vector<int> float_2_int(std::vector<float> args)
{
    std::vector<int> ret;
    for (auto arg : args)
    {
        ret.push_back(arg);
    }
    std::cout << "\nRun f2i with " << args.size();
    return ret;
}

std::vector<float> int_2_float(std::vector<int> args)
{
    std::vector<float> ret;
    for (auto arg : args)
    {
        ret.push_back(arg + 0.5f);
    }
    std::cout << "\nRun i2f with " << args.size();
    return ret;
}

int main()
{
    Executor e;
    Batcher<float, int> f2i{e, float_2_int,
        [](const std::vector<float>& args) { return args.size() >= 7; }};

    auto bla = [&](float f) ->Task {
        std::cout << "With: " << f << std::endl;
        int i = co_await f2i(i);
        std::cout << "Got: " << i << std::endl;
        co_return;
    };

    for(int i = 0; i < 7; ++i)
    {
        e.submit(bla(static_cast<float>(i)));
    }

    while(e.run_available())
    {
        
    }

    // e.submit(test1());
    // e.submit(test2());
    // e.submit(test1());
    // e.run_available();
}
