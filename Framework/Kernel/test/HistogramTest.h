#ifndef MANTID_KERNEL_HISTOGRAMTEST_H_
#define MANTID_KERNEL_HISTOGRAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Histogram/Histogram.h"

using Mantid::Kernel::Histogram;

class HistogramTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramTest *createSuite() { return new HistogramTest(); }
  static void destroySuite( HistogramTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_KERNEL_HISTOGRAMTEST_H_ */
