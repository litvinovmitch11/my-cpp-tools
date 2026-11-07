#include <coroutine>

struct Task {
  struct promise_type {
    Task get_return_object();
    std::suspend_never initial_suspend();
    std::suspend_never final_suspend() noexcept;
    void return_void();
    void unhandled_exception();
    std::suspend_always yield_value(int Value);
  };
};

Task produceValue() {
  co_await std::suspend_never{};
  co_yield 42;
  co_return;
}
