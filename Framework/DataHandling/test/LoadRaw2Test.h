#ifndef LOADRAW2TEST_H_
#define LOADRAW2TEST_H_

#include "MantidDataHandling/LoadRaw2.h"

class LoadRaw2Test : public CxxTest::TestSuite {
public:
  static LoadRaw2Test *createSuite() { return new LoadRaw2Test(); }
  static void destroySuite(LoadRaw2Test *suite) { delete suite; }

  // Test the stub remnant of version 1 of this algorithm - that it can be run
  // without setting any properties, and throws an exception.
  void testRemovedVersion1Throws() {
    Mantid::DataHandling::LoadRaw2 v2;
    v2.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(v2.initialize());
    TS_ASSERT_THROWS(v2.execute(),
                     Mantid::Kernel::Exception::NotImplementedError)
  }
};

#endif /*LOADRAWTEST_H_*/
