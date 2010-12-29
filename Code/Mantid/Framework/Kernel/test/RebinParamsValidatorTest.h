#ifndef REBINPARAMSVALIDATORTEST_H_
#define REBINPARAMSVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/RebinParamsValidator.h"

using namespace Mantid::Kernel;

class RebinParamsValidatorTest : public CxxTest::TestSuite
{
public:
  void testClone()
  {
    IValidator<std::vector<double> > *v = new RebinParamsValidator;
    IValidator<std::vector<double> > *vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv );
    TS_ASSERT( dynamic_cast<RebinParamsValidator*>(vv) );
    delete v;
    delete vv;
  }

  void testCast()
  {
    RebinParamsValidator *d = new RebinParamsValidator;
    TS_ASSERT( dynamic_cast<IValidator<std::vector<double> >*>(d) );
    delete d;
  }

  void testFailEmpty()
  {
    TS_ASSERT( ! v.isValid(std::vector<double>()).empty() );
  }

  void testFailWrongLength()
  {
    const std::vector<double> vec(6,1.0);
    TS_ASSERT( ! v.isValid(vec).empty() );
  }

  void testFailOutOfOrder()
  {
    std::vector<double> vec(5);
    vec[0] = 1.0;
    vec[1] = 0.1;
    vec[2] = 2.0;
    vec[3] = 0.2;
    vec[4] = 1.5;
    TS_ASSERT( ! v.isValid(vec).empty() );
  }

  void testFailZeroBin_or_bad_log()
  {
    //Don't give a 0 bin
    std::vector<double> vec(3);
    vec[0] = 1.0;
    vec[1] = 0.0;
    vec[2] = 2.0;
    TS_ASSERT( ! v.isValid(vec).empty() );
    //Logarithmic bin starts at 0
    vec[0] = 0.0;
    vec[1] = -1.0;
    vec[2] = 200.0;
    TS_ASSERT( ! v.isValid(vec).empty() );
    //Logarithmic bin starts at -ve number
    vec[0] = -5.0;
    vec[1] = -1.0;
    vec[2] = 10.0;
    TS_ASSERT( ! v.isValid(vec).empty() );
  }

  void testCorrect()
  {
    std::vector<double> vec(5);
    vec[0] = 1.0;
    vec[1] = 0.1;
    vec[2] = 2.0;
    vec[3] = 0.2;
    vec[4] = 2.5;
    TS_ASSERT( v.isValid(vec).empty() );
  }

private:
  RebinParamsValidator v;
};

#endif /*REBINPARAMSVALIDATORTEST_H_*/
