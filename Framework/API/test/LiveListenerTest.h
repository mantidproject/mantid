#ifndef LIVELISTENER_TEST_H_
#define LIVELISTENER_TEST_H_

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/LiveListener.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockLiveListener : public Mantid::API::LiveListener {
public:
  MockLiveListener() : Mantid::API::LiveListener() {
    // Set this flag to true for testing
    m_dataReset = true;
  }
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_CONST_METHOD0(name, std::string());
  MOCK_CONST_METHOD0(supportsHistory, bool());
  MOCK_CONST_METHOD0(buffersEvents, bool());
  MOCK_METHOD1(connect, bool(const Poco::Net::SocketAddress &));
  MOCK_METHOD1(start, void(Mantid::Types::Core::DateAndTime));
  MOCK_METHOD0(extractData, boost::shared_ptr<Mantid::API::Workspace>());
  MOCK_METHOD0(isConnected, bool());
  MOCK_METHOD0(runStatus, RunStatus());
  MOCK_CONST_METHOD0(runNumber, int());
  MOCK_METHOD1(setAlgorithm, void(const Mantid::API::IAlgorithm &));
  GNU_DIAG_ON_SUGGEST_OVERRIDE
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
};

#endif
