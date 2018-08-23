#ifndef LISTINVESTIGATIONTYPES_H_
#define LISTINVESTIGATIONTYPES_H_

#include "ICatTestHelper.h"
#include "MantidDataObjects/WorkspaceSingleValue.h" // why this is required to register table workspace.
#include "MantidICat/CatalogListInvestigationTypes.h"
#include "MantidICat/CatalogLogin.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;

class CatalogListInvestigationTypesTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogListInvestigationTypesTest *createSuite() {
    return new CatalogListInvestigationTypesTest();
  }
  static void destroySuite(CatalogListInvestigationTypesTest *suite) {
    delete suite;
  }

  /// Skip all unit tests if ICat server is down
  bool skipTests() override { return ICatTestHelper::skipTests(); }

  void testInit() {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                        "ISIS");
    TS_ASSERT_THROWS_NOTHING(invstTypesList.initialize());
    TS_ASSERT(invstTypesList.isInitialized());
  }

  void testListInvestigationTypes() {
    TS_ASSERT(ICatTestHelper::login());

    if (!invstTypesList.isInitialized())
      invstTypesList.initialize();
    // invstTypesList.setPropertyValue("OutputWorkspace","investigationtypes_list");

    TS_ASSERT_THROWS_NOTHING(invstTypesList.execute());
    TS_ASSERT(invstTypesList.isExecuted());

    ICatTestHelper::logout();
  }

private:
  CatalogListInvestigationTypes invstTypesList;
};

#endif
