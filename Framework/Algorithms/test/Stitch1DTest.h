// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_STITCH1DTEST_H_
#define MANTID_ALGORITHMS_STITCH1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/Stitch1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Algorithms::Stitch1D;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::HistogramDx;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;

double roundSix(double i) { return floor(i * 1000000. + 0.5) / 1000000.; }

namespace {
MatrixWorkspace_sptr createWorkspace(const HistogramX &xData,
                                     const HistogramY &yData,
                                     const HistogramE &eData,
                                     const HistogramDx &Dx,
                                     const int nSpec = 1) {

  Workspace2D_sptr outWS = boost::make_shared<Workspace2D>();
  outWS->initialize(nSpec, xData.size(), yData.size());
  for (int i = 0; i < nSpec; ++i) {
    outWS->mutableY(i) = yData;
    outWS->mutableE(i) = eData;
    outWS->mutableX(i) = xData;
    outWS->setPointStandardDeviations(i, Dx);
  }

  outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

  return outWS;
}

MatrixWorkspace_sptr create1DWorkspace(const HistogramX &xData,
                                       const HistogramY &yData) {
  Workspace2D_sptr outWS = boost::make_shared<Workspace2D>();
  outWS->initialize(1, xData.size(), yData.size());
  outWS->mutableY(0) = yData;
  outWS->mutableX(0) = xData;

  outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

  return outWS;
}
} // namespace

class Stitch1DTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr a;
  MatrixWorkspace_sptr b;
  std::vector<double> x;
  using ResultType = boost::tuple<MatrixWorkspace_sptr, double>;

  MatrixWorkspace_sptr make_arbitrary_point_ws() {
    const auto &x = HistogramX(3, LinearGenerator(-1., 0.2));
    const auto &y = HistogramY(3, LinearGenerator(1., 1.0));
    const auto &e = HistogramE(3, 1.);
    const auto &dx = HistogramDx(3, LinearGenerator(-3., 0.1));
    return createWorkspace(x, y, e, dx);
  }

  MatrixWorkspace_sptr make_arbitrary_histogram_ws() {
    const auto &x = HistogramX(3, LinearGenerator(-1., 0.2));
    const auto &y = HistogramY(2, LinearGenerator(1., 1.0));
    const auto &e = HistogramE(2, 1.);
    const auto &dx = HistogramDx(2, LinearGenerator(-3., 0.1));
    return createWorkspace(x, y, e, dx);
  }

  MatrixWorkspace_sptr createCosWaveWorkspace(size_t startX, size_t endX) {

    size_t count = endX - startX + 1;
    HistogramX xValues(count);
    HistogramY yValues(count - 1);
    for (size_t x = 0; x < count - 1; x++) {
      double xx = static_cast<double>(x + startX);
      xValues[x] = static_cast<double>(xx);
      yValues[x] = std::cos(xx);
    }
    xValues[count - 1] = static_cast<double>(count - 1 + startX);

    MatrixWorkspace_sptr outWS = create1DWorkspace(xValues, yValues);

    return outWS;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Stitch1DTest *createSuite() { return new Stitch1DTest(); }
  static void destroySuite(Stitch1DTest *suite) { delete suite; }

  Stitch1DTest() {

    HistogramX x(11, LinearGenerator(-1., 0.2));
    HistogramY y({0., 0., 0., 3., 3., 3., 3., 3., 3., 3.});
    HistogramE e(10, 0.);
    HistogramDx dx(10, 0.);

    // Pre-canned workspace to stitch
    this->a = createWorkspace(x, y, e, dx);

    y = {2., 2., 2., 2., 2., 2., 2., 0., 0., 0.};
    // Another pre-canned workspace to stitch
    this->b = createWorkspace(x, y, e, dx);
    this->x.assign(x.begin(), x.end());
  }

  ResultType do_stitch1D(MatrixWorkspace_sptr &lhs, MatrixWorkspace_sptr &rhs) {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  ResultType do_stitch1D(MatrixWorkspace_sptr &lhs, MatrixWorkspace_sptr &rhs,
                         const std::vector<double> &params) {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    alg.setProperty("Params", params);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  ResultType do_stitch1D(MatrixWorkspace_sptr &lhs, MatrixWorkspace_sptr &rhs,
                         bool scaleRHS, bool useManualScaleFactor,
                         const double &startOverlap, const double &endOverlap,
                         const std::vector<double> &params,
                         const double &manualScaleFactor) {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    alg.setProperty("ScaleRHSWorkspace", scaleRHS);
    alg.setProperty("UseManualScaleFactor", useManualScaleFactor);
    alg.setProperty("StartOverlap", startOverlap);
    alg.setProperty("EndOverlap", endOverlap);
    alg.setProperty("Params", params);
    alg.setProperty("ManualScaleFactor", manualScaleFactor);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  ResultType do_stitch1D(MatrixWorkspace_sptr &lhs, MatrixWorkspace_sptr &rhs,
                         const double &startOverlap, const double &endOverlap,
                         const std::vector<double> &params,
                         bool scaleRHS = true) {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    alg.setProperty("StartOverlap", startOverlap);
    alg.setProperty("EndOverlap", endOverlap);
    alg.setProperty("Params", params);
    alg.setProperty("ScaleRHSWorkspace", scaleRHS);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  ResultType do_stitch1D(MatrixWorkspace_sptr &lhs, MatrixWorkspace_sptr &rhs,
                         const double &overlap,
                         const std::vector<double> &params,
                         const bool startoverlap) {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    if (startoverlap) {
      alg.setProperty("StartOverlap", overlap);
    } else {
      alg.setProperty("EndOverlap", overlap);
    }
    alg.setProperty("Params", params);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  void test_Init() {
    Stitch1D alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_startoverlap_greater_than_end_overlap_throws() {
    std::vector<double> params = {0., 0.2, .5};
    TSM_ASSERT_THROWS(
        "Should have thrown with StartOverlap < x max",
        do_stitch1D(this->a, this->b, this->x.back(), this->x.front(), params),
        std::runtime_error &);
  }

  void test_sort_x() {
    const auto &x1 = HistogramX(3, LinearGenerator(1., 1.));
    const auto &y1 = HistogramY(3, LinearGenerator(1., 1.));
    const auto &e = HistogramE(3, LinearGenerator(7., -1.));
    const auto &dx1 = HistogramDx(3, LinearGenerator(3., -1.));
    auto point_ws_1 = createWorkspace(x1, y1, e, dx1);

    const auto &x2 = HistogramX(3, LinearGenerator(2.1, 1.));
    const auto &y2 = HistogramY(3, LinearGenerator(5., 1.));
    const auto &dx2 = HistogramDx(3, LinearGenerator(9., 0.));
    auto point_ws_2 = createWorkspace(x2, y2, e, dx2);

    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", point_ws_1);
    alg.setProperty("RHSWorkspace", point_ws_2);
    alg.setProperty("UseManualScaleFactor", true);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_const_sptr stitched = alg.getProperty("OutputWorkspace");
    const std::vector<double> x_values{1., 2., 2.1, 3., 3.1, 4.1};
    TS_ASSERT_EQUALS(stitched->x(0).rawData(), x_values);
    const std::vector<double> y_values{1., 2., 5., 3., 6., 7.};
    TS_ASSERT_EQUALS(stitched->y(0).rawData(), y_values);
    const std::vector<double> e_values{7., 6., 7., 5., 6., 5.};
    TS_ASSERT_EQUALS(stitched->e(0).rawData(), e_values);
    const std::vector<double> dx_values{3., 2., 9., 1., 9., 9.};
    TS_ASSERT_EQUALS(stitched->dx(0).rawData(), dx_values);
    double scaleFactor = alg.getProperty("OutScaleFactor");
    TS_ASSERT_EQUALS(scaleFactor, 1.); // Default scale factor
  }

  void test_point_data_input_workspace_not_modified() {
    const auto &x1 = HistogramX(3, LinearGenerator(1., 1.));
    const auto &y1 = HistogramY(3, LinearGenerator(1., 1.));
    const auto &e = HistogramE(3, LinearGenerator(7., -1.));
    const auto &dx1 = HistogramDx(3, LinearGenerator(3., -1.));
    auto ws1 = createWorkspace(x1, y1, e, dx1);
    auto ws2 = createWorkspace(x1, y1, e, dx1);
    const auto &y2 = HistogramY(3, LinearGenerator(5., 1.));
    auto ws3 = createWorkspace(x1, y2, e, dx1);
    auto ws4 = createWorkspace(x1, y2, e, dx1);
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", ws1);
    alg.setProperty("RHSWorkspace", ws3);
    alg.setProperty("UseManualScaleFactor", true);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    Mantid::Algorithms::CompareWorkspaces compare;
    compare.initialize();
    compare.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", ws2));
    TS_ASSERT(compare.execute());
    TS_ASSERT(compare.isExecuted());
    TS_ASSERT_EQUALS(compare.getPropertyValue("Result"), "1");
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace1", ws3));
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", ws4));
    TS_ASSERT(compare.execute());
    TS_ASSERT(compare.isExecuted());
    TS_ASSERT_EQUALS(compare.getPropertyValue("Result"), "1");
  }

  void test_point_data_with_dx() {
    const auto &x1 = HistogramX(3, LinearGenerator(1., 1.));
    const auto &y1 = HistogramY(3, LinearGenerator(1., 1.));
    const auto &e = HistogramE(3, 1.);
    const auto &dx1 = HistogramDx(3, LinearGenerator(3., -1.));
    auto point_ws_1 = createWorkspace(x1, y1, e, dx1);

    const auto &x2 = HistogramX(3, LinearGenerator(1.5, 1.));
    const auto &y2 = HistogramY(3, LinearGenerator(5., 1.));
    const auto &dx2 = HistogramDx(3, LinearGenerator(9., 0.));
    auto point_ws_2 = createWorkspace(x2, y2, e, dx2);

    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", point_ws_1);
    alg.setProperty("RHSWorkspace", point_ws_2);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    double scaleFactor = alg.getProperty("OutScaleFactor");
    TS_ASSERT_EQUALS(scaleFactor, 1. / 3.);
    MatrixWorkspace_const_sptr stitched = alg.getProperty("OutputWorkspace");
    const std::vector<double> x_values{1., 1.5, 2., 2.5, 3., 3.5};
    TS_ASSERT_EQUALS(stitched->x(0).rawData(), x_values);
    const std::vector<double> y_values{
        1., 5. * scaleFactor, 2., 6. * scaleFactor, 3., 7. * scaleFactor};
    TS_ASSERT_EQUALS(stitched->y(0).rawData(), y_values);
    const std::vector<double> dx_values{3., 9., 2., 9., 1., 9.};
    TS_ASSERT_EQUALS(stitched->dx(0).rawData(), dx_values);
  }

  void test_point_data_without_dx() {
    const auto &x1 = Points(3, LinearGenerator(1., 1.));
    const auto &y1 = Counts(3, LinearGenerator(1., 1.));
    Workspace2D_sptr ws1 = boost::make_shared<Workspace2D>();
    Mantid::HistogramData::Histogram histogram1(x1, y1);
    ws1->initialize(1, histogram1);
    const auto &x2 = Points(3, LinearGenerator(1.5, 1.));
    const auto &y2 = Counts(3, LinearGenerator(5., 1.));
    Mantid::HistogramData::Histogram histogram2(x2, y2);
    Workspace2D_sptr ws2 = boost::make_shared<Workspace2D>();
    ws2->initialize(1, histogram2);
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", ws1);
    alg.setProperty("RHSWorkspace", ws2);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    double scaleFactor = alg.getProperty("OutScaleFactor");
    TS_ASSERT_EQUALS(scaleFactor, 1. / 3.);
    MatrixWorkspace_const_sptr stitched = alg.getProperty("OutputWorkspace");
    const std::vector<double> x_values{1., 1.5, 2., 2.5, 3., 3.5};
    TS_ASSERT_EQUALS(stitched->x(0).rawData(), x_values);
    const std::vector<double> y_values{
        1., 5. * scaleFactor, 2., 6. * scaleFactor, 3., 7. * scaleFactor};
    TS_ASSERT_EQUALS(stitched->y(0).rawData(), y_values);
    TS_ASSERT(!stitched->hasDx(0));
  }

  void test_histogram_data_input_workspaces_not_modified() {
    auto ws1 = make_arbitrary_histogram_ws();
    auto ws3 = make_arbitrary_histogram_ws();
    const auto &x = HistogramX(3, LinearGenerator(-1., 0.2));
    const auto &y = HistogramY(2, LinearGenerator(1., 1.0));
    const auto &e = HistogramE(2, 1.);
    const auto &dx = HistogramDx(2, LinearGenerator(3., 0.1));
    auto ws2 = createWorkspace(x, y, e, dx);
    auto ws4 = createWorkspace(x, y, e, dx);
    TSM_ASSERT_THROWS_NOTHING("Histogram workspaces should pass",
                              do_stitch1D(ws2, ws1));
    Mantid::Algorithms::CompareWorkspaces compare;
    compare.initialize();
    compare.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", ws3));
    TS_ASSERT(compare.execute());
    TS_ASSERT(compare.isExecuted());
    TS_ASSERT_EQUALS(compare.getPropertyValue("Result"), "1");
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace1", ws2));
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", ws4));
    TS_ASSERT(compare.execute());
    TS_ASSERT(compare.isExecuted());
    TS_ASSERT_EQUALS(compare.getPropertyValue("Result"), "1");
  }

  void test_input_validation() {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", make_arbitrary_point_ws());
    alg.setProperty("RHSWorkspace", make_arbitrary_histogram_ws());
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());

    alg.setProperty("LHSWorkspace", make_arbitrary_histogram_ws());
    alg.setProperty("RHSWorkspace", make_arbitrary_point_ws());
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_stitching_uses_supplied_params() {
    std::vector<double> params = {-0.8, 0.2, 1.0};
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4, params);

    // Get the output workspace and look at the limits.
    const auto &xValues = ret.get<0>()->x(0);

    double xMin = xValues.front();
    double xMax = xValues.back();

    TS_ASSERT_EQUALS(xMin, -0.8);
    TS_ASSERT_EQUALS(xMax, 1.0);
  }

  void test_stitching_determines_params() {
    HistogramX x1(10, LinearGenerator(-1., 0.2));
    HistogramX x2(7, LinearGenerator(0.4, 0.2));
    HistogramY y1(9, 1.);
    HistogramY y2(6, 1.);

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    double demanded_step_size = 0.2;
    auto ret = do_stitch1D(ws1, ws2, 0.4, 1.0, {demanded_step_size});

    // Check the ranges on the output workspace against the param inputs.
    const auto &out_x_values = ret.get<0>()->x(0);
    double x_min = out_x_values.front();
    double x_max = out_x_values.back();
    double step_size = out_x_values[1] - out_x_values[0];

    TS_ASSERT_EQUALS(x_min, -1);
    TS_ASSERT_DELTA(x_max - demanded_step_size, 1.4, 0.000001);
    TS_ASSERT_DELTA(step_size, demanded_step_size, 0.000001);
  }

  void test_stitching_determines_start_and_end_overlap() {
    HistogramX x1(8, LinearGenerator(-1., 0.2));
    HistogramX x2(8, LinearGenerator(-0.4, 0.2));
    HistogramY y1({1., 1., 1., 3., 3., 3., 3.});
    HistogramY y2({1., 1., 1., 1., 3., 3., 3.});

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    std::vector<double> params = {-1.0, 0.2, 1.0};
    auto ret = do_stitch1D(ws1, ws2, params);

    const auto &stitched_y = ret.get<0>()->readY(0);
    const auto &stitched_x = ret.get<0>()->x(0);

    std::vector<size_t> overlap_indexes = std::vector<size_t>();
    for (size_t itr = 0; itr < stitched_y.size(); ++itr) {
      if (stitched_y[itr] >= 1.0009 && stitched_y[itr] <= 3.0001) {
        overlap_indexes.push_back(itr);
      }
    }

    double start_overlap_determined = stitched_x[overlap_indexes.front()];
    double end_overlap_determined = stitched_x[overlap_indexes.back()];
    TS_ASSERT_DELTA(start_overlap_determined, -0.4, 0.000000001);
    TS_ASSERT_DELTA(end_overlap_determined, 0.2, 0.000000001);
  }

  void test_stitching_forces_start_overlap() {
    HistogramX x1(8, LinearGenerator(-1, 0.2));
    HistogramX x2(8, LinearGenerator(-0.4, 0.2));
    HistogramY y1({1., 1., 1., 3., 3., 3., 3.});
    HistogramY y2({1., 1., 1., 1., 3., 3., 3.});

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    std::vector<double> params = {(-1.0), (0.2), (1.0)};
    auto ret = do_stitch1D(ws1, ws2, -0.5, params, true);

    const auto &stitched_y = ret.get<0>()->readY(0);
    const auto &stitched_x = ret.get<0>()->x(0);

    std::vector<size_t> overlap_indexes = std::vector<size_t>();
    for (size_t itr = 0; itr < stitched_y.size(); ++itr) {
      if (stitched_y[itr] >= 1.0009 && stitched_y[itr] <= 3.0001) {
        overlap_indexes.push_back(itr);
      }
    }

    double start_overlap_determined = stitched_x[overlap_indexes.front()];
    double end_overlap_determined = stitched_x[overlap_indexes.back()];
    TS_ASSERT_DELTA(start_overlap_determined, -0.4, 0.000000001);
    TS_ASSERT_DELTA(end_overlap_determined, 0.2, 0.000000001);
  }

  void test_stitching_forces_end_overlap() {
    HistogramX x1(8, LinearGenerator(-1, 0.2));
    HistogramX x2(8, LinearGenerator(-0.4, 0.2));
    HistogramY y1({1., 1., 1., 3., 3., 3., 3.});
    HistogramY y2({1., 1., 1., 1., 3., 3., 3.});

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    std::vector<double> params = {-1.0, 0.2, 1.0};
    auto ret = do_stitch1D(ws1, ws2, 0.5, params, false);

    const auto &stitched_y = ret.get<0>()->readY(0);
    const auto &stitched_x = ret.get<0>()->x(0);

    std::vector<size_t> overlap_indexes = std::vector<size_t>();
    for (size_t itr = 0; itr < stitched_y.size(); ++itr) {
      if (stitched_y[itr] >= 1.0009 && stitched_y[itr] <= 3.0001) {
        overlap_indexes.push_back(itr);
      }
    }

    double start_overlap_determined = stitched_x[overlap_indexes.front()];
    double end_overlap_determined = stitched_x[overlap_indexes.back()];
    TS_ASSERT_DELTA(start_overlap_determined, -0.4, 0.000000001);
    TS_ASSERT_DELTA(end_overlap_determined, 0.2, 0.000000001);
  }

  void test_stitching_scale_right() {
    std::vector<double> params = {0.2};
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4, params);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 2.0 / 3.0, 0.000000001);
    // Fetch the arrays from the output workspace
    auto &stitched_x = ret.get<0>()->mutableX(0);
    const auto &stitched_y = ret.get<0>()->y(0);
    const auto &stitched_e = ret.get<0>()->e(0);
    // Output Y Values should all be 2.
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr) {
      TS_ASSERT_DELTA(2., *itr, 0.000001);
    }
    // Output Error values should all be zero
    for (double temp : stitched_e) {
      TS_ASSERT_EQUALS(temp, 0.);
    }
    // Check that the output X-Values are correct.
    // truncate the input and oputput x values to 6 decimal places to
    // eliminate
    // insignificant error
    auto xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(),
                   roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x.rawData());
  }

  void test_stitching_scale_left() {
    std::vector<double> params = {0.2};
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4, params, false);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 3.0 / 2.0, 0.000000001);
    // Fetch the arrays from the output workspace
    auto &stitched_x = ret.get<0>()->mutableX(0);
    const auto &stitched_y = ret.get<0>()->y(0);
    const auto &stitched_e = ret.get<0>()->e(0);
    // Output Y Values should all be 3.
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr) {
      TS_ASSERT_DELTA(3., *itr, 0.000001);
    }
    // Output Error values should all be zero
    for (double temp : stitched_e) {
      TS_ASSERT_EQUALS(temp, 0.);
    }
    // Check that the output X-Values are correct.
    // truncate the input and oputput x values to 6 decimal places to
    // eliminate
    // insignificant error
    auto xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(),
                   roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x.rawData());
  }

  void test_stitching_manual_scale_factor_scale_right() {
    std::vector<double> params = {0.2};
    auto ret =
        do_stitch1D(this->b, this->a, true, true, -0.4, 0.4, params, 2.0 / 3.0);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 2.0 / 3.0, 0.000000001);
    // Fetch the arrays from the output workspace
    auto &stitched_x = ret.get<0>()->mutableX(0);
    const auto &stitched_y = ret.get<0>()->y(0);
    const auto &stitched_e = ret.get<0>()->e(0);
    // Output Y Values should all be 2.
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr) {
      TS_ASSERT_DELTA(2., *itr, 0.000001);
    }
    // Output Error values should all be zero
    for (double temp : stitched_e) {
      TS_ASSERT_EQUALS(temp, 0.);
    }
    // Check that the output X-Values are correct.
    // truncate the input and oputput x values to 6 decimal places to
    // eliminate
    // insignificant error
    auto xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(),
                   roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x.rawData());
  }

  void test_stitching_manual_scale_factor_scale_left() {
    std::vector<double> params = {0.2};
    auto ret = do_stitch1D(this->b, this->a, false, true, -0.4, 0.4, params,
                           3.0 / 2.0);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 3.0 / 2.0, 0.000000001);
    // Fetch the arrays from the output workspace
    auto &stitched_x = ret.get<0>()->mutableX(0);
    const auto &stitched_y = ret.get<0>()->y(0);
    const auto &stitched_e = ret.get<0>()->e(0);
    // Output Y Values should all be 3.
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr) {
      TS_ASSERT_DELTA(3., *itr, 0.000001);
    }
    // Output Error values should all be zero
    for (double temp : stitched_e) {
      TS_ASSERT_EQUALS(temp, 0);
    }
    // Check that the output X-Values are correct.
    // truncate the input and oputput x values to 6 decimal places to
    // eliminate
    // insignificant error
    auto xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(),
                   roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x.rawData());
  }

  void test_params_causing_scaling_regression_test() {
    auto lhs = createCosWaveWorkspace(0, 10);
    auto rhs = createCosWaveWorkspace(6, 20);

    auto ret = do_stitch1D(lhs, rhs);

    MatrixWorkspace_sptr outWS = ret.get<0>();
    const double scaleFactor = ret.get<1>();

    TSM_ASSERT_EQUALS("Two cosine waves in phase scale factor should be unity",
                      1.0, scaleFactor);
    const double stitchedWSFirstYValue =
        outWS->readY(0)[0];                           // Should be 1.0 at cos(0)
    const double lhsWSFirstYValue = lhs->readY(0)[0]; // Should be 1.0 at cos(0)

    TSM_ASSERT_EQUALS("No scaling of the output workspace should have occurred",
                      stitchedWSFirstYValue, lhsWSFirstYValue);
  }

  void test_has_non_zero_errors_single_spectrum() {
    HistogramX x(10, LinearGenerator(-1., 0.2));
    HistogramY y(9, 1.);
    HistogramE e(9, 1.);
    HistogramDx dx(9, 0.);

    MatrixWorkspace_sptr ws = createWorkspace(x, y, e, dx, 1);
    Stitch1D alg;
    TSM_ASSERT("All error values are non-zero", alg.hasNonzeroErrors(ws));

    // Run it again with all zeros
    e = HistogramE(9, 0.);
    ws = createWorkspace(x, y, e, dx, 1);
    TSM_ASSERT("All error values are non-zero", !alg.hasNonzeroErrors(ws));

    // Run it again with some zeros
    e.back() = 1;
    ws = createWorkspace(x, y, e, dx, 1);
    TSM_ASSERT("NOT all error values are non-zero", alg.hasNonzeroErrors(ws));
  }

  void test_has_non_zero_errors_multiple_spectrum() {
    const size_t nspectrum = 10;

    HistogramX x(10, LinearGenerator(-1., 0.2));
    HistogramY y(9, 1.);
    HistogramE e(9, 1.);
    HistogramDx dx(9, 0.);

    MatrixWorkspace_sptr ws =
        createWorkspace(x, y, e, dx, static_cast<int>(nspectrum));
    Stitch1D alg;
    TSM_ASSERT("All error values are non-zero", alg.hasNonzeroErrors(ws));

    // Run it again with all zeros
    e = HistogramE(9, 0.);
    ws = createWorkspace(x, y, e, dx, nspectrum);
    TSM_ASSERT("All error values are non-zero", !alg.hasNonzeroErrors(ws));

    // Run it again with some zeros
    e.back() = 1;
    ws = createWorkspace(x, y, e, dx, nspectrum);
    TSM_ASSERT("NOT all error values are non-zero", alg.hasNonzeroErrors(ws));
  }

  void test_patch_nan_y_value_for_scaling() {
    HistogramX x(10, LinearGenerator(0., 1.));
    HistogramY y(9, 1.);
    HistogramE e(9, 1.);
    HistogramDx dx(9, 0.);

    double nan = std::numeric_limits<double>::quiet_NaN();
    // Add a NAN
    y[5] = nan;
    MatrixWorkspace_sptr lhsWS = createWorkspace(x, y, e, dx);

    // Remove NAN
    y[5] = y[4];
    MatrixWorkspace_sptr rhsWS = createWorkspace(x, y, e, dx);

    auto ret = do_stitch1D(lhsWS, rhsWS);

    double scaleFactor = ret.get<1>();

    TSM_ASSERT("ScaleFactor should not be NAN", !std::isnan(scaleFactor));
  }

  void test_patch_inf_y_value_for_scaling() {
    HistogramX x(10, LinearGenerator(0., 1.));
    HistogramY y(9, 1.);
    HistogramE e(9, 1.);
    HistogramDx dx(9, 0.);

    double inf = std::numeric_limits<double>::infinity();
    // Add a Infinity
    y[5] = inf;
    MatrixWorkspace_sptr lhsWS = createWorkspace(x, y, e, dx);

    // Remove infinity
    y[5] = y[4];
    MatrixWorkspace_sptr rhsWS = createWorkspace(x, y, e, dx);

    auto ret = do_stitch1D(lhsWS, rhsWS);

    double scaleFactor = ret.get<1>();

    TSM_ASSERT("ScaleFactor should not be Infinity", !std::isinf(scaleFactor));
  }

  void test_reset_nans() {
    HistogramX x(10, LinearGenerator(0., 1.));
    HistogramY y(9, 1.);
    HistogramE e(9, 1.);
    HistogramDx dx(9, 0.);

    double nan = std::numeric_limits<double>::quiet_NaN();
    // Add a Infinity
    y[0] = nan;
    MatrixWorkspace_sptr lhsWS = createWorkspace(x, y, e, dx);

    // Remove infinity
    y[0] = y[1];
    MatrixWorkspace_sptr rhsWS = createWorkspace(x, y, e, dx);

    auto ret = do_stitch1D(lhsWS, rhsWS);

    MatrixWorkspace_sptr outWs = ret.get<0>();
    double scaleFactor = ret.get<1>();

    TSM_ASSERT("ScaleFactor should not be Infinity", !std::isinf(scaleFactor));

    auto outY = outWs->readY(0);
    TSM_ASSERT("Nans should be put back", std::isnan(outY[0]));
  }
};

class Stitch1DTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static Stitch1DTestPerformance *createSuite() {
    return new Stitch1DTestPerformance();
  }
  static void destroySuite(Stitch1DTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void setUp() override {
    HistogramX x1(1000, LinearGenerator(0, 0.02));
    HistogramX x2(1000, LinearGenerator(19, 0.02));
    HistogramY y1(999, 1.);
    HistogramY y2(999, 2.);

    ws1 = create1DWorkspace(x1, y1);
    ws2 = create1DWorkspace(x2, y2);
  }

  void testExec() {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", ws1);
    alg.setProperty("RHSWorkspace", ws2);
    alg.setProperty("Params", "0.2");
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
  }

private:
  MatrixWorkspace_sptr ws1;
  MatrixWorkspace_sptr ws2;
};

#endif /* MANTID_ALGORITHMS_STITCH1DTEST_H_ */
