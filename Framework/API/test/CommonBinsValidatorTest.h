#ifndef MANTID_API_COMMONBINSVALIDATORTEST_H_
#define MANTID_API_COMMONBINSVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::CommonBinsValidator;

class CommonBinsValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CommonBinsValidatorTest *createSuite() {
    return new CommonBinsValidatorTest();
  }
  static void destroySuite(CommonBinsValidatorTest *suite) { delete suite; }

  void test_empty() {
    auto ws = boost::make_shared<WorkspaceTester>();
    CommonBinsValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_zero_length_bins() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    CommonBinsValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_common_bins() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(3, 11, 10);
    for (size_t k = 0; k < 3; ++k)
      for (size_t i = 0; i < 11; ++i) {
        auto di = double(i);
        ws->dataX(k)[i] = di * (1.0 + 0.001 * di);
      }
    CommonBinsValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_diff_bins() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(3, 11, 10);
    for (size_t k = 0; k < 3; ++k)
      for (size_t i = 0; i < 11; ++i) {
        auto di = double(i + k);
        ws->dataX(k)[i] = di * (1.0 + 0.001 * di);
      }
    CommonBinsValidator validator;
    TS_ASSERT_EQUALS(
        validator.isValid(ws),
        "The workspace must have common bin boundaries for all histograms");
  }
};

#endif /* MANTID_API_COMMONBINSVALIDATORTEST_H_ */
