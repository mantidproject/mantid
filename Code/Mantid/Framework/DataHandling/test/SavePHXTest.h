#ifndef SAVEPHXTEST_H_
#define SAVEPHXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SavePHX.h"

class SavePHXTest: public CxxTest::TestSuite
{
private:
  Mantid::DataHandling::SavePHX phxSaver;
public:
  static SavePHXTest *createSuite() { return new SavePHXTest(); }
  static void destroySuite(SavePHXTest *suite) { delete suite; }

  void testAlgorithmName()
  {
    TS_ASSERT_EQUALS(phxSaver.name(), "SavePHX");
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(phxSaver.initialize());
    TS_ASSERT(phxSaver.isInitialized());
  }

  // TODO: Add proper testing...
};

#endif /*SAVEPHXTEST_H_*/
