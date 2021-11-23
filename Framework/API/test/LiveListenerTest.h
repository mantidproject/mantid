// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

  MOCK_METHOD(std::string, name, (), (const, override));
  MOCK_METHOD(bool, supportsHistory, (), (const, override));
  MOCK_METHOD(bool, buffersEvents, (), (const, override));
  MOCK_METHOD(int, runNumber, (), (const, override));

  MOCK_METHOD(void, setAlgorithm, (const Mantid::API::IAlgorithm &), (override));
  MOCK_METHOD(bool, connect, (const std::string_view), (override));
  MOCK_METHOD(void, start, (Mantid::Types::Core::DateAndTime), (override));
  MOCK_METHOD(std::shared_ptr<Mantid::API::Workspace>, extractData, (), (override));
  MOCK_METHOD(bool, isConnected, (), (override));
  MOCK_METHOD(RunStatus, runStatus, (), (override));
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
