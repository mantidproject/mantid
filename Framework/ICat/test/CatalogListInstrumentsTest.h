// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidICat/CatalogListInstruments.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace ICatTestHelper;

class CatalogListInstrumentsTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogListInstrumentsTest *createSuite() { return new CatalogListInstrumentsTest(); }
  static void destroySuite(CatalogListInstrumentsTest *suite) { delete suite; }

  CatalogListInstrumentsTest() : m_fakeLogin(std::make_unique<FakeICatLogin>()) {}

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(instrList.initialize());
    TS_ASSERT(instrList.isInitialized());
  }

  void testListInstruments() {
    if (!instrList.isInitialized())
      instrList.initialize();
    // instrList.setPropertyValue("OutputWorkspace","instrument_list");

    TS_ASSERT_THROWS_NOTHING(instrList.execute());
    TS_ASSERT(instrList.isExecuted());
  }

private:
  CatalogListInstruments instrList;
  std::unique_ptr<FakeICatLogin> m_fakeLogin;
};
