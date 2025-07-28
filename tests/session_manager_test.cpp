#include "../src/session_manager.h"

#include <gtest/gtest.h>
#include <pgw_utils/logger_config.h>

#include <chrono>
#include <memory>

using namespace protei;

class SessionManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    spdlog::set_level(spdlog::level::warn);
    blacklist = {"111112222233333", "444445555566666"};

    callback = [this](const SessionEvent& event) {
      events.emplace_back(event);
    };

    session_manager = std::make_unique<SessionManager>(
        std::chrono::seconds(1),  // Short timeout for testing
        10,                       // Shutdown rate
        blacklist, callback);
  }

  std::vector<SessionEvent> events;
  SessionManager::SessionEventCallback callback;
  std::unordered_set<std::string> blacklist;
  std::shared_ptr<SessionManager> session_manager;
};

TEST_F(SessionManagerTest, CreateNewSession) {
  auto result = session_manager->create_session("123456789012345");

  EXPECT_EQ(result, SessionManager::CreateResult::CREATED);
  EXPECT_TRUE(session_manager->session_exists("123456789012345"));

  EXPECT_EQ(events.size(), 1);
  EXPECT_EQ(events[0].imsi, "123456789012345");
  EXPECT_EQ(events[0].action, SessionAction::CREATED);
}

TEST_F(SessionManagerTest, CreateExistingSession) {
  auto result1 = session_manager->create_session("123456789012345");
  EXPECT_EQ(result1, SessionManager::CreateResult::CREATED);

  auto result2 = session_manager->create_session("123456789012345");
  EXPECT_EQ(result2, SessionManager::CreateResult::ALREADY_EXISTS);

  EXPECT_TRUE(session_manager->session_exists("123456789012345"));

  EXPECT_EQ(events.size(), 1);
}

TEST_F(SessionManagerTest, BlacklistedSession) {
  auto result = session_manager->create_session("111112222233333");
  EXPECT_EQ(result, SessionManager::CreateResult::BLACKLISTED);
  EXPECT_FALSE(session_manager->session_exists("111112222233333"));

  // Check CDR event for rejection
  ASSERT_EQ(events.size(), 1);
  EXPECT_EQ(events[0].imsi, "111112222233333");
  EXPECT_EQ(events[0].action, SessionAction::REJECTED);
}

TEST_F(SessionManagerTest, InvalidIMSI) {
  auto result = session_manager->create_session("invalid_imsi_abc");
  EXPECT_EQ(result, SessionManager::CreateResult::BLACKLISTED);
  EXPECT_FALSE(session_manager->session_exists("invalid_imsi_abc"));
}

TEST_F(SessionManagerTest, SessionTimeout) {
  // Create session
  session_manager->create_session("123456789012345");
  EXPECT_TRUE(session_manager->session_exists("123456789012345"));

  // Wait for timeout (timeout is 1 second + cleanup interval)
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  EXPECT_FALSE(session_manager->session_exists("123456789012345"));

  // Should have CREATED and TIMEOUT events
  EXPECT_GE(events.size(), 2);
  bool found_timeout = false;
  for (const auto& event : events) {
    if (event.action == SessionAction::TIMEOUT) {
      found_timeout = true;
      break;
    }
  }
  EXPECT_TRUE(found_timeout);
}

TEST_F(SessionManagerTest, ConcurrentAccess) {
  const int num_threads = 10;
  const int sessions_per_thread = 50;
  std::vector<std::thread> threads;

  // Start multiple threads creating sessions concurrently
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, sessions_per_thread]() {
      for (int i = 0; i < sessions_per_thread; ++i) {
        std::string imsi =
            std::to_string(t) + "34567890123" + std::to_string(i);
        session_manager->create_session(imsi);
      }
    });
  }

  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }

  // Verify that all sessions were created
  for (int t = 0; t < num_threads; ++t) {
    for (int i = 0; i < sessions_per_thread; ++i) {
      std::string imsi = std::to_string(t) + "34567890123" + std::to_string(i);
      EXPECT_TRUE(session_manager->session_exists(imsi));
    }
  }

  // Should have num_threads * sessions_per_thread CREATED events
  EXPECT_EQ(events.size(), num_threads * sessions_per_thread);
}