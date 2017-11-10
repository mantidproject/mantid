#ifndef MANTID_API_HISTOGRAMVALIDATORTEST_H_
#define MANTID_API_HISTOGRAMVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/HistogramValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::HistogramValidator;

class HistogramValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramValidatorTest *createSuite() {
    return new HistogramValidatorTest();
  }
  static void destroySuite(HistogramValidatorTest *suite) { delete suite; }

  void test_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    HistogramValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_fail() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 10, 10);
    HistogramValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws),
                     "The workspace must contain histogram data");
  }
};

#endif /* MANTID_API_HISTOGRAMVALIDATORTEST_H_ */
