#ifndef MANTID_API_RAWCOUNTVALIDATORTEST_H_
#define MANTID_API_RAWCOUNTVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RawCountValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::RawCountValidator;

class RawCountValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RawCountValidatorTest *createSuite() {
    return new RawCountValidatorTest();
  }
  static void destroySuite(RawCountValidatorTest *suite) { delete suite; }

  void test_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 1, 1);
    RawCountValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_fail() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 1, 1);
    ws->setDistribution(true);
    RawCountValidator validator;
    TS_ASSERT_EQUALS(
        validator.isValid(ws),
        "A workspace containing numbers of counts is required here");
  }
};

#endif /* MANTID_API_RAWCOUNTVALIDATORTEST_H_ */