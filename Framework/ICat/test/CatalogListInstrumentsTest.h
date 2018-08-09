#ifndef LISTINSTRUMENTS_H_
#define LISTINSTRUMENTS_H_

#include "ICatTestHelper.h"
#include "MantidDataObjects/WorkspaceSingleValue.h" // why this is required to register table workspace.
#include "MantidICat/CatalogListInstruments.h"
#include "MantidICat/CatalogLogin.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;

class CatalogListInstrumentsTest : public CxxTest::TestSuite {
public:
  /// Skip all unit tests if ICat server is down
  bool skipTests() override { return ICatTestHelper::skipTests(); }

  void testInit() {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                        "ISIS");
    TS_ASSERT_THROWS_NOTHING(instrList.initialize());
    TS_ASSERT(instrList.isInitialized());
  }

  void testListInstruments() {
    TS_ASSERT(ICatTestHelper::login());

    if (!instrList.isInitialized())
      instrList.initialize();
    // instrList.setPropertyValue("OutputWorkspace","instrument_list");

    TS_ASSERT_THROWS_NOTHING(instrList.execute());
    TS_ASSERT(instrList.isExecuted());

    ICatTestHelper::logout();
  }

private:
  CatalogListInstruments instrList;
};

#endif
