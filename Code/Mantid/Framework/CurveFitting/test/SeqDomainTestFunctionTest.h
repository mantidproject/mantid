#ifndef MANTID_CURVEFITTING_SEQDOMAINTESTFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_SEQDOMAINTESTFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/SeqDomainTestFunction.h"

using Mantid::CurveFitting::SeqDomainTestFunction;

class SeqDomainTestFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SeqDomainTestFunctionTest *createSuite() { return new SeqDomainTestFunctionTest(); }
  static void destroySuite( SeqDomainTestFunctionTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CURVEFITTING_SEQDOMAINTESTFUNCTIONTEST_H_ */