// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/LiveListener.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <vector>

namespace Poco::Net {
// Poco does not define this include the Poco::Net so GMock can't find it.
inline std::ostream &operator<<(std::ostream &os, const Poco::Net::SocketAddress &address) {
  return (os << "SocketAddress mock @ " << reinterpret_cast<const void *>(&address));
}
} // namespace Poco::Net

class MockLiveListener : public Mantid::API::LiveListener {
public:
  MockLiveListener() : Mantid::API::LiveListener() {
    // Set this flag to true for testing
    m_dataReset = true;
  }
  MOCK_METHOD(std::string, name, (), (const, override));
  MOCK_METHOD(bool, supportsHistory, (), (const, override));
  MOCK_METHOD(bool, buffersEvents, (), (const, override));
  MOCK_METHOD(bool, connect, (const Poco::Net::SocketAddress &), (override));
  MOCK_METHOD(void, start, (Mantid::Types::Core::DateAndTime), (override));
  MOCK_METHOD(std::shared_ptr<Mantid::API::Workspace>, doExtractData, (), (override));
  MOCK_METHOD(bool, isConnected, (), (override));
  MOCK_METHOD(Mantid::API::ILiveListener::RunStatus, runState, (), (const, override));
  MOCK_METHOD(Mantid::API::ListenerState, listenerState, (), (const, override));
  MOCK_METHOD(int, runNumber, (), (const, override));
  MOCK_METHOD(void, setAlgorithm, (const Mantid::API::IAlgorithm &), (override));
};

// ---------------------------------------------------------------------------
// Minimal test subclass for the new pure-getter default tests.
// Drives runState() and lastTransition() from controllable members.
// ---------------------------------------------------------------------------
class TestableListener : public Mantid::API::LiveListener {
public:
  // Controllable state
  Mantid::API::ILiveListener::RunStatus m_runState{Mantid::API::ILiveListener::NoRun};
  std::optional<Mantid::API::ILiveListener::RunStatus> m_lastTransition{std::nullopt};
  bool m_isPaused{false};
  Mantid::API::ListenerState m_listenerState{Mantid::API::ListenerState::Disconnected};

  // Pure-getter overrides
  RunStatus runState() const override { return m_runState; }
  bool isPaused() const override { return m_isPaused; }
  Mantid::API::ListenerState listenerState() const override { return m_listenerState; }
  std::optional<RunStatus> lastTransition() const override { return m_lastTransition; }

  // Required pure virtuals (not exercised in these tests)
  std::string name() const override { return "TestableListener"; }
  bool supportsHistory() const override { return false; }
  bool buffersEvents() const override { return false; }
  bool connect(const Poco::Net::SocketAddress &) override { return true; }
  void start(Mantid::Types::Core::DateAndTime) override {}
  std::shared_ptr<Mantid::API::Workspace> doExtractData() override { return nullptr; }
  bool isConnected() override { return true; }
  int runNumber() const override { return 0; }
  void setAlgorithm(const Mantid::API::IAlgorithm &) override {}
};

class LiveListenerTest : public CxxTest::TestSuite {
public:
  void testDataReset() {
    using Mantid::API::ILiveListener;
    ILiveListener *l = new MockLiveListener;
    // On the first call it should be true
    TS_ASSERT(l->dataReset())
    // On subsequent calls it should be false
    TS_ASSERT(!l->dataReset())
    TS_ASSERT(!l->dataReset())
    delete l;
  }

  // --- New tests for sub-spec 01 base-interface additions ---

  void test_base_runStatus_default_returns_runState_when_no_edge() {
    // When lastTransition() is nullopt, runStatus() shim returns runState().
    TestableListener listener;
    listener.m_runState = Mantid::API::ILiveListener::Running;
    listener.m_lastTransition = std::nullopt;

    GNU_DIAG_OFF("deprecated-declarations")
    const auto status = listener.runStatus();
    GNU_DIAG_ON("deprecated-declarations")

    TS_ASSERT_EQUALS(status, Mantid::API::ILiveListener::Running);
  }

  void test_base_runStatus_default_returns_lastTransition_when_present() {
    // When lastTransition() has a value, runStatus() shim returns that edge.
    TestableListener listener;
    listener.m_runState = Mantid::API::ILiveListener::Running;
    listener.m_lastTransition = Mantid::API::ILiveListener::BeginRun;

    GNU_DIAG_OFF("deprecated-declarations")
    const auto status = listener.runStatus();
    GNU_DIAG_ON("deprecated-declarations")

    TS_ASSERT_EQUALS(status, Mantid::API::ILiveListener::BeginRun);
  }

  void test_base_defaults_are_const_correct() {
    // All new pure getters must be callable on a const reference.
    const TestableListener listener;
    // Verify default values via const ref — this is also a compile-time check.
    TS_ASSERT_EQUALS(listener.runState(), Mantid::API::ILiveListener::NoRun);
    TS_ASSERT_EQUALS(listener.isPaused(), false);
    TS_ASSERT_EQUALS(listener.listenerState(), Mantid::API::ListenerState::Disconnected);
    TS_ASSERT(!listener.lastTransition().has_value());
  }

  // --- New tests for sub-spec 02b template-method extractData() ---

  void test_extractData_calls_hooks_in_order_pre_do_after() {
    // Records the call order in a shared counter.
    class OrderedListener : public Mantid::API::LiveListener {
    public:
      std::vector<int> calls;
      std::string name() const override { return "OrderedListener"; }
      bool supportsHistory() const override { return false; }
      bool buffersEvents() const override { return false; }
      bool connect(const Poco::Net::SocketAddress &) override { return true; }
      void start(Mantid::Types::Core::DateAndTime) override {}
      bool isConnected() override { return true; }
      Mantid::API::ListenerState listenerState() const override { return Mantid::API::ListenerState::Connected; }
      int runNumber() const override { return 0; }
      void setAlgorithm(const Mantid::API::IAlgorithm &) override {}

    protected:
      void onBeforeExtract() override { calls.push_back(1); }
      std::shared_ptr<Mantid::API::Workspace> doExtractData() override {
        calls.push_back(2);
        return nullptr;
      }
      void onAfterExtract() override { calls.push_back(3); }
    };

    OrderedListener listener;
    (void)listener.extractData();
    TS_ASSERT_EQUALS(listener.calls.size(), 3u);
    TS_ASSERT_EQUALS(listener.calls[0], 1);
    TS_ASSERT_EQUALS(listener.calls[1], 2);
    TS_ASSERT_EQUALS(listener.calls[2], 3);
  }

  void test_onAfterExtract_not_called_when_onBeforeExtract_throws() {
    class ThrowingHookListener : public Mantid::API::LiveListener {
    public:
      int doExtractCalls{0};
      int afterExtractCalls{0};
      std::string name() const override { return "ThrowingHookListener"; }
      bool supportsHistory() const override { return false; }
      bool buffersEvents() const override { return false; }
      bool connect(const Poco::Net::SocketAddress &) override { return true; }
      void start(Mantid::Types::Core::DateAndTime) override {}
      bool isConnected() override { return true; }
      Mantid::API::ListenerState listenerState() const override { return Mantid::API::ListenerState::Connected; }
      int runNumber() const override { return 0; }
      void setAlgorithm(const Mantid::API::IAlgorithm &) override {}

    protected:
      void onBeforeExtract() override { throw std::runtime_error("hook aborted"); }
      std::shared_ptr<Mantid::API::Workspace> doExtractData() override {
        ++doExtractCalls;
        return nullptr;
      }
      void onAfterExtract() override { ++afterExtractCalls; }
    };

    ThrowingHookListener listener;
    TS_ASSERT_THROWS(listener.extractData(), const std::runtime_error &);
    TS_ASSERT_EQUALS(listener.doExtractCalls, 0);
    TS_ASSERT_EQUALS(listener.afterExtractCalls, 0);
  }

  void test_onAfterExtract_not_called_when_doExtractData_throws_NotYet() {
    class NotYet final : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
    };

    class SideEffectListener : public Mantid::API::LiveListener {
    public:
      int hookCounter{0};
      int afterExtractCalls{0};
      std::string name() const override { return "SideEffectListener"; }
      bool supportsHistory() const override { return false; }
      bool buffersEvents() const override { return false; }
      bool connect(const Poco::Net::SocketAddress &) override { return true; }
      void start(Mantid::Types::Core::DateAndTime) override {}
      bool isConnected() override { return true; }
      Mantid::API::ListenerState listenerState() const override { return Mantid::API::ListenerState::Connected; }
      int runNumber() const override { return 0; }
      void setAlgorithm(const Mantid::API::IAlgorithm &) override {}

    protected:
      void onBeforeExtract() override { ++hookCounter; }
      std::shared_ptr<Mantid::API::Workspace> doExtractData() override { throw NotYet("not yet"); }
      void onAfterExtract() override { ++afterExtractCalls; }
    };

    SideEffectListener listener;
    TS_ASSERT_THROWS(listener.extractData(), const NotYet &);
    TS_ASSERT_EQUALS(listener.hookCounter, 1);
    TS_ASSERT_EQUALS(listener.afterExtractCalls, 0);
    TS_ASSERT_THROWS(listener.extractData(), const NotYet &);
    TS_ASSERT_EQUALS(listener.hookCounter, 2);
    TS_ASSERT_EQUALS(listener.afterExtractCalls, 0);
  }

  void test_throw_from_onAfterExtract_propagates_and_workspace_is_dropped() {
    class ThrowingAfterListener : public Mantid::API::LiveListener {
    public:
      std::weak_ptr<Mantid::API::Workspace> extractedWorkspace;

      std::string name() const override { return "ThrowingAfterListener"; }
      bool supportsHistory() const override { return false; }
      bool buffersEvents() const override { return false; }
      bool connect(const Poco::Net::SocketAddress &) override { return true; }
      void start(Mantid::Types::Core::DateAndTime) override {}
      bool isConnected() override { return true; }
      Mantid::API::ListenerState listenerState() const override { return Mantid::API::ListenerState::Connected; }
      int runNumber() const override { return 0; }
      void setAlgorithm(const Mantid::API::IAlgorithm &) override {}

    protected:
      std::shared_ptr<Mantid::API::Workspace> doExtractData() override {
        auto workspace = std::make_shared<FakeWorkspace>();
        extractedWorkspace = workspace;
        return workspace;
      }
      void onAfterExtract() override { throw std::runtime_error("after extract failed"); }
    };

    ThrowingAfterListener listener;
    TS_ASSERT_THROWS(listener.extractData(), const std::runtime_error &);
    TS_ASSERT(listener.extractedWorkspace.expired());
  }
};
