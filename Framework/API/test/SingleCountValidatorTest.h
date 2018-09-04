#ifndef MANTID_API_SINGLECOUNTVALIDATORTEST_H_
#define MANTID_API_SINGLECOUNTVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SingleCountValidator.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::HistogramData;
using Mantid::API::SingleCountValidator;

class SingleCountValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SingleCountValidatorTest *createSuite() {
    return new SingleCountValidatorTest();
  }
  static void destroySuite(SingleCountValidatorTest *suite) { delete suite; }

  void test_single_count_workspace_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    SingleCountValidator validator(true);
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_non_single_count_workspace_failure() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 3, 2);
    SingleCountValidator validator(true);
    TS_ASSERT_EQUALS(
        validator.isValid(ws),
        "The workspace must contain single counts for all spectra");
  }

  void test_single_count_workspace_failure() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    SingleCountValidator validator(false);
    TS_ASSERT_EQUALS(validator.isValid(ws),
                     "The workspace must not contain single counts");
  }

  void test_non_single_count_workspace_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 3, 2);
    SingleCountValidator validator(false);
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  // The next two tests serve as a warning - only the first bin is checked!
  void test_variable_bin_workspace_actually_succeeds() {
    auto ws = boost::make_shared<VariableBinThrowingTester>();
    ws->initialize(2, 3, 2);
    BinEdges bins{-1.0, 1.0};
    Counts counts{1.0};
    Histogram hist(bins, counts);

    ws->setHistogram(0, hist);

    SingleCountValidator validator(true);
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_variable_bin_workspace_actually_fails() {
    auto ws = boost::make_shared<VariableBinThrowingTester>();
    ws->initialize(2, 3, 2);
    BinEdges bins{-1.0, 1.0};
    Counts counts{1.0};
    Histogram hist(bins, counts);

    ws->setHistogram(0, hist);

    SingleCountValidator validator(false);
    TS_ASSERT_EQUALS(validator.isValid(ws),
                     "The workspace must not contain single counts");
  }
};

#endif /* MANTID_API_SINGLECOUNTVALIDATORTEST_H_ */
