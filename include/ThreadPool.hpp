#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * @brief Small fixed-size pool of jthread workers for asynchronous jobs.
 *
 * Workers drain queued tasks and are stopped automatically when the pool is
 * destroyed. Tasks already queued are allowed to finish during shutdown.
 */
class ThreadPool {
public:
  /**
   * @brief Starts the requested number of worker threads.
   * @param threads Number of workers to create.
   */
  explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) {
    for (size_t i{0}; i < threads; ++i) {
      workers_.emplace_back([this](std::stop_token stop_tok) {
        while (!stop_tok.stop_requested()) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->queue_mutex_);
            this->cv_.wait(lock, [this, &stop_tok] {
              return stop_tok.stop_requested() || !this->tasks_.empty();
            });

            if (stop_tok.stop_requested() && this->tasks_.empty())
              return;

            if (this->tasks_.empty())
              continue;

            task = std::move(this->tasks_.front());
            this->tasks_.pop();
          }
          task();
        }
      });
    }
  }

  /**
   * @brief Adds a callable to the queue and returns its future result.
   * @tparam F Callable type.
   * @tparam Args Callable argument types.
   * @param f Callable to execute.
   * @param args Arguments forwarded to the callable.
   * @return Future containing the callable's return value or exception.
   */
  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return res;
  }

  /** @brief Requests worker shutdown and joins all workers. */
  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      for (auto &worker : workers_) {
        worker.request_stop();
      }
    }
    cv_.notify_all();
  }

private:
  std::vector<std::jthread> workers_{};
  std::mutex queue_mutex_{};
  std::queue<std::function<void()>> tasks_{};
  std::condition_variable cv_{};
};

#endif
