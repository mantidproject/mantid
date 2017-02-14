#ifndef MANTID_DATAOBJECTS_VIRTUAL_PEAKTEST_H_
#define MANTID_DATAOBJECTS_VIRTUAL_PEAKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/VirtualPeak.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class VirtualPeakTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VirtualPeakTest *createSuite() { return new VirtualPeakTest(); }
  static void destroySuite(VirtualPeakTest *suite) { delete suite; }

  void test_constructor() {
    VirtualPeak p;
  }

  void test_getDetPos() {
    VirtualPeak p;
    auto pos = p.getDetPos();
    TS_ASSERT_EQUALS(pos, V3D(0,0,0))
  }

};


#endif /* MANTID_DATAOBJECTS_VIRTUAL_PEAKTEST_H_ */
