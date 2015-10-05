#ifndef LOADRAWTEST_H_
#define LOADRAWTEST_H_

#include "MantidDataHandling/LoadRaw.h"

class LoadRawTest : public CxxTest::TestSuite {
public:
  static LoadRawTest *createSuite() { return new LoadRawTest(); }
  static void destroySuite(LoadRawTest *suite) { delete suite; }

  // Test the stub remnant of version 1 of this algorithm - that it can be run
  // without setting any properties, and throws an exception.
  void testRemovedVersion1Throws() {
    Mantid::DataHandling::LoadRaw v1;
    v1.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(v1.initialize());
    TS_ASSERT_THROWS(v1.execute(),
                     Mantid::Kernel::Exception::NotImplementedError)
  }
};

#endif /*LOADRAWTEST_H_*/
