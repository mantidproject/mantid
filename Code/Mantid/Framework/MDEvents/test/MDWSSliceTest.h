#ifndef MANTID_MDWS_SLICE_H_
#define MANTID_MDWS_SLICE_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/MDWSSlice.h"

class MDWSSliceTest : public CxxTest::TestSuite
{
    MDWSSliceTest slice;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDWSSliceTest *createSuite() { return new MDWSSliceTest(); }
  static void destroySuite( MDWSSliceTest *suite ) { delete suite; }

  void test_constructor()
  {
      TS_WARN(" Nothing to test yet");
  }
};
#endif