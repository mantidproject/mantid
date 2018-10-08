// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MYDATASEARCH_H_
#define MYDATASEARCH_H_

#include "ICatTestHelper.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogMyDataSearch.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;

class CatalogMyDataSearchTest : public CxxTest::TestSuite {
public:
  /// Skip all unit tests if ICat server is down
  bool skipTests() override { return ICatTestHelper::skipTests(); }

  void testInit() {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                        "ISIS");
    CatalogMyDataSearch mydata;
    TS_ASSERT_THROWS_NOTHING(mydata.initialize());
    TS_ASSERT(mydata.isInitialized());
  }
  void testMyDataSearch() {
    CatalogMyDataSearch mydata;

    TS_ASSERT(ICatTestHelper::login());

    if (!mydata.isInitialized())
      mydata.initialize();

    mydata.setPropertyValue("OutputWorkspace", "MyInvestigations");

    TS_ASSERT_THROWS_NOTHING(mydata.execute());
    TS_ASSERT(mydata.isExecuted());

    ICatTestHelper::logout();
  }
};
#endif
