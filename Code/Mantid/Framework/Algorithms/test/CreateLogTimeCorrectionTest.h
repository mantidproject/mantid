#ifndef MANTID_ALGORITHMS_CREATELOGTIMECORRECTIONTEST_H_
#define MANTID_ALGORITHMS_CREATELOGTIMECORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateLogTimeCorrection.h"

using Mantid::Algorithms::CreateLogTimeCorrection;

class CreateLogTimeCorrectionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateLogTimeCorrectionTest *createSuite() { return new CreateLogTimeCorrectionTest(); }
  static void destroySuite( CreateLogTimeCorrectionTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_ALGORITHMS_CREATELOGTIMECORRECTIONTEST_H_ */