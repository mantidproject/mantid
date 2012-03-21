#ifndef MANTID_MDWS_SLICE_H_
#define MANTID_MDWS_SLICE_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/MDWSSliceDescr.h"

class MDWSSliceDescrTest : public CxxTest::TestSuite
{
    //MDWSSliceTest slice;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDWSSliceDescrTest *createSuite() { return new MDWSSliceDescrTest(); }
  static void destroySuite( MDWSSliceDescrTest *suite ) { delete suite; }

  void test_constructor()
  {
      TS_WARN(" Nothing to test yet");
  }
};
#endif