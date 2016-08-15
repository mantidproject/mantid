#ifndef ILIVELISTENER_TEST_H_
#define ILIVELISTENER_TEST_H_

#include "MantidAPI/ILiveListener.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::API;

class MockILiveListener : public ILiveListener {
public:
  MockILiveListener() : ILiveListener() {
    // Set this flag to true for testing
    m_dataReset = true;
  }
  GCC_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_CONST_METHOD0(name, std::string());
  MOCK_CONST_METHOD0(supportsHistory, bool());
  MOCK_CONST_METHOD0(buffersEvents, bool());
  MOCK_METHOD1(connect, bool(const Poco::Net::SocketAddress &));
  MOCK_METHOD1(start, void(Mantid::Kernel::DateAndTime));
  MOCK_METHOD0(extractData, boost::shared_ptr<Workspace>());
  MOCK_METHOD0(isConnected, bool());
  MOCK_METHOD0(runStatus, RunStatus());
  MOCK_CONST_METHOD0(runNumber, int());
  GCC_DIAG_ON_SUGGEST_OVERRIDE
};

class ILiveListenerTest : public CxxTest::TestSuite {
public:
  void testDataReset() {
    ILiveListener *l = new MockILiveListener;
    // On the first call it should be true
    TS_ASSERT(l->dataReset())
    // On subsequent calls it should be false
    TS_ASSERT(!l->dataReset())
    TS_ASSERT(!l->dataReset())
    delete l;
  }
};

#endif
