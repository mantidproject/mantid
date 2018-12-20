#ifndef MANTID_API_EQUALBINSIZESVALIDATORTEST_H_
#define MANTID_API_EQUALBINSIZESVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/EqualBinSizesValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::EqualBinSizesValidator;

class EqualBinSizesValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EqualBinSizesValidatorTest *createSuite() {
    return new EqualBinSizesValidatorTest();
  }
  static void destroySuite(EqualBinSizesValidatorTest *suite) { delete suite; }

  void test_null() {
    EqualBinSizesValidator val(0.1);
    TS_ASSERT_DIFFERS(val.isValid(nullptr), "");
  }

  void test_noBins() {
    using namespace Mantid::HistogramData;
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 1, 1);
    ws->setHistogram(0, Points(0), Counts(0));
    EqualBinSizesValidator val(0.1);
    TS_ASSERT_EQUALS(val.isValid(ws), "Enter a workspace with some data in it");
  }

  void test_noCommonBins() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 3, 3);
    Mantid::MantidVec xData{1, 2, 3};
    ws->setPoints(1, xData);
    EqualBinSizesValidator val(0.1);
    TS_ASSERT_EQUALS(
        val.isValid(ws),
        "The workspace must have common bin boundaries for all histograms");
  }

  void test_equalBinSizes() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 3, 3);
    EqualBinSizesValidator val(0.1);
    TS_ASSERT_EQUALS(val.isValid(ws), "");
  }

  void test_unequalBinSizes() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(1, 3, 3);
    Mantid::MantidVec xData{1, 2, 5};
    ws->setPoints(0, xData);
    EqualBinSizesValidator val(0.1);
    TS_ASSERT_EQUALS(
        val.isValid(ws),
        "X axis must be linear (all bins must have the same width)");
  }
};

#endif /* MANTID_API_EQUALBINSIZESVALIDATORTEST_H_ */
