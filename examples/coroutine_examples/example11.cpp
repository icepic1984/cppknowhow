// https://www.jeremyong.com/cpp/2021/01/04/cpp20-coroutines-a-minimal-async-framework/
#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <iostream>
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

auto dbg = [](const char* s) { std::cout << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

template <typename T>
class Queue
{
public:
    /**
     * Pushes element on queue
     *
     * @param e  the element
     */
    void push(const T& e)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(e);
        lock.unlock();
        m_cond.notify_one();
    };

    void push(T&& e)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(std::move(e));
        lock.unlock();
        m_cond.notify_one();
    };

    /**
     *
     * @return true if queue is empty, false otherwise
     */
    bool empty() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
     * If queue is not empty, sets reference parameter to
     * first element of queue, removes this element and returns
     * true. Otherwise returns false.
     *
     * @param e reference to the first element
     *
     * @return true if queue is empty, false otherwise
     */
    bool tryPop(const T& e)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return false;
        }
        e = m_queue.front();
        m_queue.pop();
        return true;
    }

    /**
     * If queue is not empty, removes the first element from queue and
     * returns this element to the caller. If queue is empty, sleeps
     * until an new element is inserted.
     *
     *
     * @return first element of the queue
     */
    T waitAndPop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty())
        {
            m_cond.wait(lock);
        }
        T data = std::move(m_queue.front());
        m_queue.pop();
        return data;
    }

    /**
     * If queue is not empy, sets data to the first element and removes
     * the first element from queue. If queue is empty, sleeps until new
     * element is inserted.
     *
     * @param e reference to the first element
     */
    void waitAndPop(T& e)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty())
        {
            m_cond.wait(lock);
        }
        e = m_queue.front();
        m_queue.pop();
    }

    /**
     * Clears queue
     *
     */
    void clear()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        std::queue<T>().swap(m_queue);
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
};

class ThreadRunner
{
public:
    using Func = std::function<void()>;

    static ThreadRunner& instance()
    {
        static ThreadRunner runner;
        return runner;
    }

    ThreadRunner()
        : m_thread([this] { this->run(); })
    {
    }

    ~ThreadRunner()
    {
        enqueue([&] { m_done = true; });
        m_thread.join();
    }

    ThreadRunner(const ThreadRunner&) = delete;

    ThreadRunner& operator=(const ThreadRunner&) = delete;

    void enqueue(Func f)
    {
        m_queue.push(std::move(f));
    }

private:
    void run()
    {
        while (!m_done)
        {
            Func f = m_queue.waitAndPop();
            f();
        }
    }

    Queue<Func> m_queue;

    bool m_done = false;

    std::thread m_thread;
};

class CTP
{
public:
    static CTP& instance()
    {
        static CTP ctp{2};
        return ctp;
    }

    CTP(int thread_count)
    {
    }

    void enqueue(std::coroutine_handle<> coroutine)
    {
        std::cout << "Enqueue" << std::endl;
    }

private:
    std::thread m_thread;
};

struct event_awaiter
{
    HANDLE event;
    bool await_ready() const noexcept
    {
        DBG;
        // Check if we need to bother suspending at all by seeing if the
        // event was already signaled with a non-blocking wait.
        return WaitForSingleObject(event, 0) == WAIT_OBJECT_0;
    }

    void await_suspend(std::coroutine_handle<> coroutine) const noexcept
    {
        // The coroutine handle passed here can be copied elsewhere and resumed
        // when the event is signaled. Here, we spawn a dedicated thread for
        // demonstration purposes, but you should have a dedicated low-priority
        // thread to queue waiting events to.
        DBG;
        //std::thread thread{[coroutine, this]() noexcept {
           // WaitForSingleObject(event, INFINITE);

            // The CTP will call coroutine.resume() on an available thread
            // now
            ThreadRunner::instance().enqueue(
                [coroutine, event= event]() { WaitForSingleObject(event, INFINITE);
            coroutine.resume(); });
        //}};
        //thread.detach();
    }

    void await_resume() noexcept
    {
        DBG;
        // This is called after the coroutine is resumed in the async thread
        printf("Event signaled, resuming on thread %i\n",
            std::this_thread::get_id());
    }

    event_awaiter await_transform(HANDLE event)
    {
        DBG;
        return {event};
    };
};

inline auto suspend() noexcept
{
    struct awaiter
    {
        // Unlike the OS event case, there's no case where we suspend and
        // the work is immediately ready
        bool await_ready() const noexcept
        {
            return false;
        }

        // Since await_ready() always returns false, when suspend is called,
        // we will always immediately suspend and call this function (which
        // enqueues the coroutine for immediate reactivation on a different
        // thread)
        void await_suspend(std::coroutine_handle<> coroutine) noexcept
        {
            CTP::instance().enqueue(coroutine);
        }

        void await_resume() const noexcept
        {
            printf("Suspended task now running on thread $i\n",
                std::this_thread::get_id());
        }
    };
    return awaiter{};
}

HANDLE createTimer()
{
    HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    LARGE_INTEGER liDueTime;

    liDueTime.QuadPart = -100000000LL;
    // Set a timer to wait for 10 seconds.
    SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
    return hTimer;
}

template <typename T>
struct task
{
    // The return type of a coroutine must contain a nested struct or type
    // alias called `promise_type`
    struct promise_type
    {
        // Keep a coroutine handle referring to the parent coroutine if any.
        // That is, if we co_await a coroutine within another coroutine,
        // this handle will be used to continue working from where we left
        // off.
        std::coroutine_handle<> precursor;

        // Place to hold the results produced by the coroutine
        T data;

        // Invoked when we first enter a coroutine. We initialize the
        // precursor handle with a resume point from where the task is
        // ultimately suspended
        task get_return_object() noexcept
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // When the caller enters the coroutine, we have the option to
        // suspend immediately. Let's choose not to do that here
        std::suspend_never initial_suspend() const noexcept
        {
            return {};
        }

        // If an exception was thrown in the coroutine body, we would handle
        // it here
        void unhandled_exception()
        {
        }

        // The coroutine is about to complete (via co_return or reaching the
        // end of the coroutine body). The awaiter returned here defines
        // what happens next
        auto final_suspend() const noexcept
        {
            struct awaiter
            {
                // Return false here to return control to the thread's event
                // loop. Remember that we're running on some async thread at
                // this point.
                bool await_ready() const noexcept
                {
                    return false;
                }

                void await_resume() const noexcept
                {
                }

                // Returning a coroutine handle here resumes the coroutine
                // it refers to (needed for continuation handling). If we
                // wanted, we could instead enqueue that coroutine handle
                // instead of immediately resuming it by enqueuing it and
                // returning void.
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) noexcept
                {
                    auto precursor = h.promise().precursor;
                    if (precursor)
                    {
                        return precursor;
                    }
                    return std::noop_coroutine();
                }
            };
            return awaiter{};
        }

        // When the coroutine co_returns a value, this method is used to
        // publish the result
        void return_value(T value) noexcept
        {
            data = std::move(value);
        }
    };

    // The following methods make our task type conform to the awaitable
    // concept, so we can co_await for a task to complete

    bool await_ready() const noexcept
    {
        // No need to suspend if this task has no outstanding work
        return handle.done();
    }

    T await_resume() const noexcept
    {
        // The returned value here is what `co_await our_task` evaluates to
        return std::move(handle.promise().data);
    }

    void await_suspend(std::coroutine_handle<> coroutine) const noexcept
    {
        // The coroutine itself is being suspended (async work can beget
        // other async work) Record the argument as the continuation point
        // when this is resumed later. See the final_suspend awaiter on the
        // promise_type above for where this gets used
        handle.promise().precursor = coroutine;
    }

    // This handle is assigned to when the coroutine itself is suspended
    // (see await_suspend above)
    std::coroutine_handle<promise_type> handle;
};

task<int> test()
{
    HANDLE event = createTimer();
    co_await event_awaiter{event};
}

int main()
{
    std::cout << "Start thread" << std::endl;
    ThreadRunner::instance();
    std::cout << "Start timer" << std::endl;
    auto f = test();
    std::cout << "Go on" << std::endl;
    // Retrieve an event that has yet to be signaled
    // HANDLE event = createTimer();

    // Assuming the event is unsignaled, this task will now suspend
    // (await_suspend will be called above). The thread that this code
    // originally ran on is now free to resume a different coroutine
    // enqueued earlier (if any) or simply wait until a task is ready.
    // co_await event;

    // At this point, await_resume() above has been called and we are now
    // running on a potentially different CTP thread.

    // Wait for the timer.

    // if (WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0)
    //    printf("WaitForSingleObject failed (%d)\n", GetLastError());
    // else
    //     printf("Timer was signaled.\n");
    std::this_thread::sleep_for(std::chrono::seconds(20));
    return 0;
}
