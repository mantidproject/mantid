#ifndef GETINVESTIGATION_H_
#define GETINVESTIGATION_H_

#include "ICatTestHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidICat/CatalogGetDataFiles.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogSearch.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;

class CatalogGetDataFilesTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogGetDataFilesTest *createSuite() {
    return new CatalogGetDataFilesTest();
  }
  static void destroySuite(CatalogGetDataFilesTest *suite) { delete suite; }

  /// Skip all unit tests if ICat server is down
  bool skipTests() override { return ICatTestHelper::skipTests(); }

  CatalogGetDataFilesTest() { API::FrameworkManager::Instance(); }

  void testInit() {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                        "ISIS");
    TS_ASSERT_THROWS_NOTHING(invstObj.initialize());
    TS_ASSERT(invstObj.isInitialized());
  }

  void testgetDataFilesExecutes() {
    TS_ASSERT(ICatTestHelper::login());

    if (!invstObj.isInitialized())
      invstObj.initialize();
    invstObj.setPropertyValue("InvestigationId", "12576918");
    invstObj.setPropertyValue(
        "OutputWorkspace", "investigation"); // selected invesigation data files

    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());

    ICatTestHelper::logout();
  }

private:
  CatalogGetDataFiles invstObj;
};
#endif
