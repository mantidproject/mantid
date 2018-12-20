#ifndef REBINPARAMSVALIDATORTEST_H_
#define REBINPARAMSVALIDATORTEST_H_

#include "MantidKernel/RebinParamsValidator.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class RebinParamsValidatorTest : public CxxTest::TestSuite {
public:
  void testClone() {
    IValidator_sptr v = boost::make_shared<RebinParamsValidator>();
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS(v, vv);
    TS_ASSERT(boost::dynamic_pointer_cast<RebinParamsValidator>(vv));
  }

  void testCast() {
    RebinParamsValidator *d = new RebinParamsValidator;
    TS_ASSERT(dynamic_cast<IValidator *>(d));
    delete d;
  }

  void testFailEmpty() {
    TS_ASSERT_EQUALS(standardValidator.isValid(std::vector<double>()),
                     "Enter values for this property");
  }

  void testSucceedEmpty() {
    RebinParamsValidator allowEmptyValidator = RebinParamsValidator(true);
    TS_ASSERT(allowEmptyValidator.isValid(std::vector<double>()).empty());
  }

  void testFailWrongLength() {
    const std::vector<double> vec(6, 1.0);
    TS_ASSERT(!standardValidator.isValid(vec).empty());
  }

  void testFailOutOfOrder() {
    std::vector<double> vec(5);
    vec[0] = 1.0;
    vec[1] = 0.1;
    vec[2] = 2.0;
    vec[3] = 0.2;
    vec[4] = 1.5;
    TS_ASSERT_EQUALS(
        standardValidator.isValid(vec),
        "Bin boundary values must be given in order of increasing value");
  }

  void testFailZeroBin_or_bad_log() {
    // Don't give a 0 bin
    std::vector<double> vec(3);
    vec[0] = 1.0;
    vec[1] = 0.0;
    vec[2] = 2.0;
    TS_ASSERT(!standardValidator.isValid(vec).empty());
    // Logarithmic bin starts at 0
    vec[0] = 0.0;
    vec[1] = -1.0;
    vec[2] = 200.0;
    TS_ASSERT(!standardValidator.isValid(vec).empty());
    // Logarithmic bin starts at -ve number
    vec[0] = -5.0;
    vec[1] = -1.0;
    vec[2] = 10.0;
    TS_ASSERT(!standardValidator.isValid(vec).empty());
  }

  void testCorrect() {
    std::vector<double> vec(5);
    vec[0] = 1.0;
    vec[1] = 0.1;
    vec[2] = 2.0;
    vec[3] = 0.2;
    vec[4] = 2.5;
    TS_ASSERT(standardValidator.isValid(vec).empty());
  }

  void testRange() {
    auto allowRangeValidator = RebinParamsValidator(true, true);
    std::vector<double> vec(2);
    vec[0] = 0.0;
    vec[1] = 1.0;
    TS_ASSERT(allowRangeValidator.isValid(vec).empty());
  }

  void testRangeWrongOrder() {
    auto allowRangeValidator = RebinParamsValidator(true, true);
    std::vector<double> vec(2);
    vec[0] = 1.0;
    vec[1] = 0.0;
    TS_ASSERT_EQUALS(
        allowRangeValidator.isValid(vec),
        "When giving a range the second value must be larger than the first");
  }

private:
  RebinParamsValidator standardValidator;
};

#endif /*REBINPARAMSVALIDATORTEST_H_*/
