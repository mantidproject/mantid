#ifndef GETIDATASETS_H_
#define GETIDATASETS_H_

#include "ICatTestHelper.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidICat/CatalogGetDataSets.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogSearch.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;

class CatalogGetDataSetsTest : public CxxTest::TestSuite {
public:
  /// Skip all unit tests if ICat server is down
  bool skipTests() override { return ICatTestHelper::skipTests(); }

  void testInit() {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                        "ISIS");

    TS_ASSERT_THROWS_NOTHING(datasets.initialize());
    TS_ASSERT(datasets.isInitialized());
  }

  void testgetDataFilesExecutes() {
    TS_ASSERT(ICatTestHelper::login());

    if (!datasets.isInitialized())
      datasets.initialize();
    datasets.setPropertyValue("InvestigationId", "12576918");
    datasets.setPropertyValue("OutputWorkspace",
                              "investigation"); // selected invesigation

    TS_ASSERT_THROWS_NOTHING(datasets.execute());
    TS_ASSERT(datasets.isExecuted());

    ICatTestHelper::logout();
  }

private:
  CatalogGetDataSets datasets;
};
#endif
