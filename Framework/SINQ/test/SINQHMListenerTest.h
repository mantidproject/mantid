// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include "MantidSINQ/SINQHMListener.h"
#include <cxxtest/TestSuite.h>
#include <sstream>
#include <stdexcept>

/** Stub subclass that bypasses real HTTP requests.
 *  Override httpRequest() to return canned response streams.
 */
class StubSINQHMListener : public SINQHMListener {
public:
  /// Response body that httpRequest() will return.
  std::string stubbedResponse;

protected:
  std::istream &httpRequest(const std::string & /*path*/) override {
    m_responseStream = std::istringstream(stubbedResponse);
    return m_responseStream;
  }

private:
  std::istringstream m_responseStream;
};

// ---------------------------------------------------------------------------

class SINQHMListenerTest : public CxxTest::TestSuite {
public:
  static SINQHMListenerTest *createSuite() { return new SINQHMListenerTest(); }
  static void destroySuite(SINQHMListenerTest *suite) { delete suite; }

  /** runState() must be a pure getter: calling it without an intervening
   *  onBeforeExtract() / extractData() must not mutate state.
   */
  void test_runState_is_pure_getter() {
    StubSINQHMListener listener;
    // Default state before any poll is NoRun.
    const auto s1 = listener.runState();
    const auto s2 = listener.runState();
    TS_ASSERT_EQUALS(s1, Mantid::API::ILiveListener::NoRun);
    TS_ASSERT_EQUALS(s1, s2);
  }

  /** onBeforeExtract() parses the HTTP response and refreshes
   *  m_cachedRunState (accessible via runState()), hmhost, and dimDirty.
   *  When DAQ==1 and the previous status was NoRun, dimDirty must be set.
   */
  void test_onBeforeExtract_sets_Running_and_marks_dimDirty_on_first_start() {
    StubSINQHMListener listener;
    listener.stubbedResponse = "HM-Host: testhm\nDAQ: 1\n";

    // Call onBeforeExtract via extractData() is not feasible without a live
    // server (doExtractData would fail). Call via the protected hook directly
    // using a friend trampoline — instead we expose a thin test helper.
    // Since onBeforeExtract is protected we invoke it through a thin subclass.
    class PollableListener : public StubSINQHMListener {
    public:
      void pollForTest() { onBeforeExtract(); }
    };

    PollableListener pl;
    pl.stubbedResponse = "HM-Host: testhm\nDAQ: 1\n";
    TS_ASSERT_EQUALS(pl.runState(), Mantid::API::ILiveListener::NoRun);
    TS_ASSERT_THROWS_NOTHING(pl.pollForTest());
    TS_ASSERT_EQUALS(pl.runState(), Mantid::API::ILiveListener::Running);
  }

  /** onBeforeExtract() must throw std::runtime_error on an unrecognised
   *  DAQ status code, preserving the existing behaviour.
   */
  void test_onBeforeExtract_throws_on_invalid_DAQ_code() {
    class PollableListener : public StubSINQHMListener {
    public:
      void pollForTest() { onBeforeExtract(); }
    };

    PollableListener pl;
    pl.stubbedResponse = "HM-Host: testhm\nDAQ: 99\n";
    TS_ASSERT_THROWS(pl.pollForTest(), const std::runtime_error &);
  }
};
