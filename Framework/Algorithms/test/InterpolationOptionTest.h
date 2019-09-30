// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_INTERPOLATIONOPTIONTEST_H_
#define MANTID_ALGORITHMS_INTERPOLATIONOPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/PropertyWithValue.h"

using Mantid::Algorithms::InterpolationOption;
using namespace Mantid::HistogramData;

class InterpolationOptionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InterpolationOptionTest *createSuite() {
    return new InterpolationOptionTest();
  }
  static void destroySuite(InterpolationOptionTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Property_Defaults_To_Linear_Interpolation() {
    InterpolationOption interpolateOpt;
    auto prop = interpolateOpt.property();

    TS_ASSERT(prop);
    TS_ASSERT_EQUALS("Interpolation", prop->name());
    TS_ASSERT_EQUALS("Linear", prop->getDefault());
  }

  void test_Documentation_Is_Not_Empty() {
    InterpolationOption interpolateOpt;

    TS_ASSERT(!interpolateOpt.propertyDoc().empty())
  }

  void test_Apply_With_Linear_Succeeds() {
    using namespace Mantid::HistogramData;
    InterpolationOption interpolateOpt;

    Histogram inOut(Points(7, LinearGenerator(0, 0.5)),
                    Counts({-3, 0, -4, 0, 4, 0, 3}));
    Histogram input(inOut);
    interpolateOpt.applyInplace(inOut, 2);

    const std::vector<double> expectedY = {-3, -3.5, -4, 0, 4, 3.5, 3};
    checkData(input, inOut, expectedY);
  }

  void test_Apply_With_CSpline_Succeeds() {
    InterpolationOption interpolateOptEnum;
    // Set by enum
    interpolateOptEnum.set(InterpolationOption::Value::CSpline);

    Histogram inOut(Points(7, LinearGenerator(0, 0.5)),
                    Counts({-3, 0, -4, 0, 4, 0, 3}));
    const Histogram input(inOut);
    interpolateOptEnum.applyInplace(inOut, 2);

    const std::vector<double> expectedY = {-3, -4.625, -4, 0., 4, 4.625, 3};
    checkData(input, inOut, expectedY);

    // Set by string
    InterpolationOption interpolateOptStr;
    interpolateOptStr.set("CSpline");

    Histogram inOutStr(input);
    interpolateOptStr.applyInplace(inOutStr, 2);

    checkData(input, inOutStr, expectedY);
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_set_From_String_Throws_With_Unknown_Type() {
    InterpolationOption interpolateOpt;
    TS_ASSERT_THROWS(interpolateOpt.set("Unknown"),
                     const std::invalid_argument &);
  }

  void test_set_From_String_Throws_With_Empty_String() {
    InterpolationOption interpolateOpt;
    TS_ASSERT_THROWS(interpolateOpt.set(""), const std::invalid_argument &);
  }

  void test_validateInputSize() {
    using namespace Mantid::HistogramData;
    auto minSize = minSizeForCSplineInterpolation();
    InterpolationOption opt;
    opt.set("CSpline");
    TS_ASSERT(opt.validateInputSize(minSize).empty())
    TS_ASSERT(!opt.validateInputSize(minSize - 1).empty())
    minSize = minSizeForLinearInterpolation();
    opt.set("Linear");
    TS_ASSERT(opt.validateInputSize(minSize).empty())
    TS_ASSERT(!opt.validateInputSize(minSize - 1).empty())
  }

private:
  void checkData(const Histogram &input, const Histogram &output,
                 const std::vector<double> &expectedY) {
    TS_ASSERT_EQUALS(input.x(), output.x());
    TS_ASSERT_EQUALS(input.xMode(), output.xMode());
    TS_ASSERT_EQUALS(input.yMode(), output.yMode());
    const auto &outY = output.y();
    for (size_t i = 0; i < expectedY.size(); ++i) {
      TS_ASSERT_DELTA(expectedY[i], outY[i], 1e-14);
    }
  }
};

#endif /* MANTID_ALGORITHMS_INTERPOLATIONOPTIONTEST_H_ */
