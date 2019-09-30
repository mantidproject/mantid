// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_INTERPOLATETEST_H_
#define MANTID_HISTOGRAMDATA_INTERPOLATETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidHistogramData/LinearGenerator.h"

using namespace Mantid::HistogramData;

class InterpolateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InterpolateTest *createSuite() { return new InterpolateTest(); }
  static void destroySuite(InterpolateTest *suite) { delete suite; }

  // ---------------------------------------------------------------------------
  // Success cases - linear in-place no copy
  // ---------------------------------------------------------------------------
  void test_interpolateLinearInPlaceDoes_Not_Copy() {
    Histogram input(Points(5, LinearGenerator(0, 0.5)), {-2, 0, 0, 0, 2});
    auto xAddrBefore = &input.x();
    auto yAddrBefore = &input.y();
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(input, 4));

    TS_ASSERT_EQUALS(xAddrBefore, &input.x());
    TS_ASSERT_EQUALS(yAddrBefore, &input.y());
  }

  // ---------------------------------------------------------------------------
  // Success cases - linear, point X data
  // ---------------------------------------------------------------------------
  void test_interpolateLinearPointDataSet_Stepsize_One_Less_Point_Size() {
    Histogram input(Points(5, LinearGenerator(0, 0.5)), {-2, 0, 0, 0, 2});
    auto output = interpolateLinear(input, 4);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-2., -1., 0., 1., 2.};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(inOut, 4));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  void test_interpolateLinearPointDataSet_Even_StepSize() {
    Histogram input(Points(5, LinearGenerator(0, 0.5)), {-2, 0, 0.5, 0, 2});
    auto output = interpolateLinear(input, 2);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-2., -0.75, 0.5, 1.25, 2.};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(inOut, 2));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  void test_interpolateLinearPointDataSet_Odd_StepSize() {
    Histogram input(Points(5, LinearGenerator(0, 0.5)), {-2, 0, 0.0, 0.5, 2});
    auto output = interpolateLinear(input, 3);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-2., -2 + (2.5 / 1.5) * 0.5, -1. / 3., 0.5,
                                     2.};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(inOut, 3));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  void test_interpolateLinearInplace_interpolates() {
    Histogram input(Points(2, LinearGenerator(0, 1.0)), {-0.72, -0.72});
    Histogram output(Points(1, LinearGenerator(0.5, 1.0)), {0.0});
    interpolateLinearInplace(input, output);
    TS_ASSERT_EQUALS(output.y()[0], -0.72)
  }

  // ---------------------------------------------------------------------------
  // Success cases - cspline in-place no copy
  // ---------------------------------------------------------------------------
  void test_interpolateCSplineInPlaceDoes_Not_Copy() {
    Histogram input(Points(7, LinearGenerator(0, 0.5)),
                    {-3, 0, -1, 0, 1, 0, 3});
    auto xAddrBefore = &input.x();
    auto yAddrBefore = &input.y();
    TS_ASSERT_THROWS_NOTHING(interpolateCSplineInplace(input, 2));

    TS_ASSERT_EQUALS(xAddrBefore, &input.x());
    TS_ASSERT_EQUALS(yAddrBefore, &input.y());
  }

  // ---------------------------------------------------------------------------
  // Success cases - cspline, point X data
  // ---------------------------------------------------------------------------
  void test_interpolateCSplinePointDataSet_Minimum_Calculated_Points() {
    Histogram input(Points(7, LinearGenerator(0, 0.5)),
                    Counts({-3, 0, -4, 0, 4, 0, 3}));
    auto output = interpolateCSpline(input, 2);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-3, -4.625, -4, 0., 4, 4.625, 3};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateCSplineInplace(inOut, 2));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  void test_interpolateCSplineInplace_interpolates() {
    Histogram input(Points(3, LinearGenerator(0, 1.0)), {-0.72, -0.72, -0.72});
    Histogram output(Points(1, LinearGenerator(0.1, 1.0)), {0.0});
    interpolateCSplineInplace(input, output);
    TS_ASSERT_EQUALS(output.y()[0], -0.72);
  }

  // ---------------------------------------------------------------------------
  // Success cases - linear edge X data
  // ---------------------------------------------------------------------------
  void test_interpolateLinearEdgeDataSet_Stepsize_One_Less_Point_Size() {
    Histogram input(BinEdges(6, LinearGenerator(-0.25, 0.5)),
                    Counts({-2, 0, 0, 0, 2}));
    auto output = interpolateLinear(input, 4);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-2., -1., 0., 1., 2.};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(inOut, 4));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  void test_interpolateLinearEdgeDataSet_Even_StepSize() {
    Histogram input(BinEdges(6, LinearGenerator(-0.25, 0.5)),
                    {-2, 0, 0.5, 0, 2});
    auto output = interpolateLinear(input, 2);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-2., -0.75, 0.5, 1.25, 2.};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(inOut, 2));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  void test_interpolateLinearEdgeDataSet_Odd_StepSize() {
    Histogram input(BinEdges(6, LinearGenerator(-0.25, 0.5)),
                    {-2, 0, 0.0, 0.5, 2});
    Histogram output = interpolateLinear(input, 3);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-2., -2 + (2.5 / 1.5) * 0.5, -1. / 3., 0.5,
                                     2.};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(inOut, 3));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  // ---------------------------------------------------------------------------
  // Success cases - cspline edge X data
  // ---------------------------------------------------------------------------
  void test_interpolateCSplineEdgeDataSet_Minimum_Calculated_Points() {
    Histogram input(BinEdges(8, LinearGenerator(-0.25, 0.5)),
                    Counts({-3, 0, -1, 0, 1, 0, 3}));
    auto output = interpolateCSpline(input, 2);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-3, -2, -1, 0, 1, 2, 3};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateCSplineInplace(inOut, 2));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  // ---------------------------------------------------------------------------
  // Success cases - Point data with frequencies
  //                 single test case as whitebox testing tells us the algorithm
  //                 is the same
  // ---------------------------------------------------------------------------

  void test_interpolateLinearPointFrequencyData_Stepsize_One_Less_Point_Size() {
    Histogram input(Points(5, LinearGenerator(0., 0.5)),
                    Frequencies({-2, 0, 0, 0, 2}));
    auto output = interpolateLinear(input, 4);

    checkSizesUnchanged(input, output);
    std::vector<double> expectedY = {-2., -1., 0., 1., 2.};
    checkData(input, output, expectedY);

    // Inplace
    Histogram inOut(input);
    TS_ASSERT_THROWS_NOTHING(interpolateLinearInplace(inOut, 4));

    checkSizesUnchanged(input, inOut);
    checkData(input, inOut, expectedY);
  }

  // ---------------------------------------------------------------------------
  // Common checking code
  // ---------------------------------------------------------------------------
  void checkSizesUnchanged(const Histogram &input, const Histogram &output) {
    TS_ASSERT_EQUALS(input.y().size(), output.y().size());
    TS_ASSERT_EQUALS(input.x().size(), output.x().size());
  }

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

  // ---------------------------------------------------------------------------
  // Failure cases - linear
  // ---------------------------------------------------------------------------
  void test_interpolatelinear_throws_for_undefined_ymode_type() {
    TS_ASSERT_THROWS(
        interpolateLinear(Histogram(Points(10, LinearGenerator(0, 0.5))), 10),
        const std::runtime_error &);
  }

  void test_interpolatelinear_throws_if_number_points_less_than_3() {
    TS_ASSERT_THROWS(
        interpolateLinear(Histogram(Points(2, LinearGenerator(0, 0.5))), 1),
        const std::runtime_error &);
    TS_ASSERT_THROWS(
        interpolateLinear(Histogram(Points(2, LinearGenerator(0, 0.5))), 1),
        const std::runtime_error &);
  }

  void test_interpolatelinearinplace_throws_if_input_has_less_than_2_points() {
    Histogram input(Points(1, LinearGenerator(0.1, 0.1)));
    Histogram output(Points(1, LinearGenerator(0.1, 0.1)));
    TS_ASSERT_THROWS(interpolateLinearInplace(input, output),
                     const std::runtime_error &)
  }

  void
  test_interpolatelinear_throws_if_stepsize_greater_or_equal_number_points() {
    TS_ASSERT_THROWS(
        interpolateLinear(Histogram(Points(6, LinearGenerator(0, 0.5))), 6),
        const std::runtime_error &);
    TS_ASSERT_THROWS(
        interpolateLinear(Histogram(Points(6, LinearGenerator(0, 0.5))), 7),
        const std::runtime_error &);
  }

  // ---------------------------------------------------------------------------
  // Failure cases - cspline
  // ---------------------------------------------------------------------------
  void test_interpolatecspline_throws_for_undefined_ymode_type() {
    TS_ASSERT_THROWS(
        interpolateCSpline(Histogram(Points(10, LinearGenerator(0, 0.5))), 10),
        const std::runtime_error &);
  }

  void test_interpolatecspline_throws_if_less_than_4_calculated_points() {
    TS_ASSERT_THROWS(
        interpolateCSpline(Histogram(Points(2, LinearGenerator(0, 0.5))), 1),
        const std::runtime_error &);
    TS_ASSERT_THROWS(
        interpolateCSpline(Histogram(Points(3, LinearGenerator(0, 0.5))), 1),
        const std::runtime_error &);
  }

  void test_interpolatecsplineinplace_throws_if_input_has_less_than_3_points() {
    Histogram input(Points(2, LinearGenerator(0, 1.0)));
    Histogram output(Points(5, LinearGenerator(0.1, 0.1)));
    TS_ASSERT_THROWS(interpolateCSplineInplace(input, output),
                     const std::runtime_error &)
  }

  void
  test_interpolatecspline_throws_if_stepsize_greater_or_equal_number_points() {
    TS_ASSERT_THROWS(
        interpolateCSpline(Histogram(Points(6, LinearGenerator(0, 0.5))), 6),
        const std::runtime_error &);
    TS_ASSERT_THROWS(
        interpolateCSpline(Histogram(Points(6, LinearGenerator(0, 0.5))), 7),
        const std::runtime_error &);
  }
};

// ---------------------------------------------------------------------------
// Performance test
// ---------------------------------------------------------------------------
class InterpolateTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InterpolateTestPerformance *createSuite() {
    return new InterpolateTestPerformance();
  }
  static void destroySuite(InterpolateTestPerformance *suite) { delete suite; }

  InterpolateTestPerformance()
      : hist(BinEdges(binSize, LinearGenerator(0, 1))) {
    Counts counts(binSize - 1, LinearGenerator(10, 0.1));
    hist.setCounts(counts);
  }

  void testInterpolateLinearSmallStep() {
    for (size_t i = 0; i < nIters; i++)
      interpolateLinear(hist, 2);
  }

  void testInterpolateSplineSmallStep() {
    for (size_t i = 0; i < nIters; i++)
      interpolateCSpline(hist, 2);
  }

private:
  const size_t binSize = 10000;
  const size_t nIters = 5000;
  Histogram hist;
};

#endif /* MANTID_HISTOGRAMDATA_INTERPOLATETEST_H_ */
