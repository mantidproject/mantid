#ifndef MANTID_API_SPECTRAAXISVALIDATORTEST_H_
#define MANTID_API_SPECTRAAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::SpectraAxisValidator;

class SpectraAxisValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectraAxisValidatorTest *createSuite() {
    return new SpectraAxisValidatorTest();
  }
  static void destroySuite(SpectraAxisValidatorTest *suite) { delete suite; }

  void test_fail() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    auto newAxis = new NumericAxis(2);
    ws->replaceAxis(1, newAxis);
    SpectraAxisValidator validator;
    TS_ASSERT_EQUALS(
        validator.isValid(ws),
        "A workspace with axis being Spectra Number is required here.");
  }

  void test_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    SpectraAxisValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }
};

#endif /* MANTID_API_SPECTRAAXISVALIDATORTEST_H_ */
