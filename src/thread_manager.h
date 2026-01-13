#ifndef CLARKESIM_SRC_COMMON_THREAD_MANAGER_C_
#define CLARKESIM_SRC_COMMON_THREAD_MANAGER_C_

#include <algorithm>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <map>
#include <atomic>
#include <shared_mutex>


/// @brief Simple class to disallow copying of derived classes
class Uncopyable {
   public:
    Uncopyable() {}

   private:
    Uncopyable(const Uncopyable&);
    Uncopyable& operator=(Uncopyable);
};  // class Uncopyable

/// @brief Simple class to disallow moving of derived classes
class Unmovable {
   public:
    Unmovable() {}

   private:
    Unmovable(Unmovable&&);
    Unmovable& operator=(Unmovable&&);
};  // class Unmovable

/// @brief a class with thread-safe utilities for synchronized derived classes
///
/// there are no 'hard' controls on synchronization. the implemented of derived
/// classes must use the supplied utility member functions to enable
/// synchronization. if these member functions are not used, inheritance from
/// this class is useless
class ThreadSafe {
   public:
    ThreadSafe() {}

   protected:
    /// @brief returns an object that will lock this instance until the object
    /// destructs
    ///
    /// @returns an RAII-style object that holds a lock on the calling object's
    /// mutex
    inline std::scoped_lock<std::shared_mutex> WriteLock() const {
        return std::scoped_lock<std::shared_mutex>(m_);
    }

    /// @brief returns an object that will share-lock this instance until the
    /// object destructs
    ///
    /// @returns an RAII-style object that holds a shared lock on the calling
    /// object's mutex
    inline std::shared_lock<std::shared_mutex> ReadLock() const {
        return std::shared_lock<std::shared_mutex>(m_);
    }

   private:
    mutable std::shared_mutex m_{};
};  // class ThreadSafe


/// @brief  Decouples sim executor type from api
/// concretely SingleThreadExecutor, potential multithreaded executor or synchronous in the future
class SimExecutor {
public:
    using Task = std::function<void()>;
    virtual ~SimExecutor() = default;
    virtual void Post(Task task) = 0;
};

class SingleThreadExecutor : public SimExecutor {
public:
    SingleThreadExecutor()
        : running_(true),
          thread_(&SingleThreadExecutor::Run, this) {}

    ~SingleThreadExecutor() override {
        Stop();
        if (thread_.joinable())
            thread_.join();
    }

    void Post(Task task) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(task));
        }
        cv_.notify_one();
    }

    void Stop() {
        running_ = false;
        cv_.notify_all();
    }

private:
    void Run() {
        while (true) {
            Task task;

            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [&] {
                    return !queue_.empty() || !running_;
                });

                if (!running_ && queue_.empty())
                    return;

                task = std::move(queue_.front());
                queue_.pop();
            }

            task();
        }
    }

    std::queue<Task> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
    std::thread thread_;
};

/// @brief simple class for managing unnamed threads. threads added to the
/// ThreadManager are moved, so thread variables added are no longer valid
/// except in this object.
class ThreadManager : private Uncopyable,
                      private Unmovable {
public:
    ThreadManager() {
        console_ = std::make_unique<SingleThreadExecutor>();
    }
    ~ThreadManager() { Join(); }

    /// @brief add the thread represented by t to the threads managed by this
    /// object
    /// @param t the thread to add. added threads are std::move'd
    void AddThread(std::thread t) { threads_.push_back(std::move(t)); }

    void Join() {
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
    }

    inline void PostTelem(std::reference_wrapper<IConsole> obj, ITelemetryPrint data) {
        console_->Post([obj, data = std::move(data)] {
            auto con = obj.get().addTelemetry(std::move(data));
            obj.get().printTelemTable(con);
        });
    }

    inline void PostStatus(std::reference_wrapper<IConsole> obj, IStatusPrint data) {
        console_->Post([obj, data = std::move(data)] {
            obj.get().newStatus(data);
        });
    }

    inline void UpdateStatus(std::reference_wrapper<IConsole> obj, IStatusPrint data) {
        console_->Post([obj, data = std::move(data)] {
            obj.get().updateStatus(data.id, data);
        });
    }

private:
    std::vector<std::thread> threads_{};
    std::unique_ptr<SimExecutor> console_;

};
#endif  // CLARKESIM_SRC_COMMON_THREAD_MANAGER_H_