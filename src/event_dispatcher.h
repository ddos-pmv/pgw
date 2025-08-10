#pragma once



#include "concurrentqueue.h"
#include "event.h"
#include "session_manager.h"

namespace protei {

class EventDispatcher {
 public:
  EventDispatcher(size_t thread_count,
                  moodycamel::ConcurrentQueue<Event>& queue,
                  std::shared_ptr<SessionManager> session_manager);

  ~EventDispatcher();

  void notify_event_available();

 private:
  void handleEvent(Event&& event);

  void handleUdpEvent(UdpEvent&& event);

  void stop();
  moodycamel::ConcurrentQueue<Event>& queue_;
  std::atomic_bool stop_;
  std::vector<std::thread> thread_pool_;
  std::shared_ptr<SessionManager> session_manager_;
  std::mutex notify_mutex_;
  std::condition_variable notify_cv_;
  std::atomic<bool> event_available_{false};
};

}  // namespace protei