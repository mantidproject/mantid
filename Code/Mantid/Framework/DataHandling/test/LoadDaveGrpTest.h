#ifndef LOADDAVEGRPTEST_H_
#define LOADDAVEGRPTEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadDaveGrp.h"

using namespace Mantid::DataHandling;

class LoadDaveGrpTest : public CxxTest::TestSuite
{
public:
  void testLoading()
  {
    LoadDaveGrp loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename",
        "DaveAscii.grp"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace",
        "dave_grp"));
    loader.execute();

    TS_ASSERT_EQUALS(loader.isExecuted(), true);
  }
};

#endif // LOADDAVEGRPTEST_H_
