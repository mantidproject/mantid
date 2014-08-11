#ifndef MANTID_ALGORITHMS_STITCH1DTEST_H_
#define MANTID_ALGORITHMS_STITCH1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Stitch1D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include <algorithm>
#include <math.h>
#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/math/special_functions.hpp>
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace boost::assign;
using Mantid::Algorithms::Stitch1D;
using Mantid::MantidVec;
using namespace Mantid::DataObjects;

double roundSix(double i)
{
  return floor(i * 1000000 + 0.5) / 1000000;
}

class Stitch1DTest: public CxxTest::TestSuite
{
private:

  template<typename T>
  class LinearSequence
  {
  private:
    const T m_start;
    const T m_step;
    T m_stepNumber;
  public:
    LinearSequence(const T& start, const T& step) :
        m_start(start), m_step(step), m_stepNumber(T(0))
    {
    }

    T operator()()
    {
      T result = m_start + (m_step * m_stepNumber);
      m_stepNumber += 1;
      return result;
    }

  };

  MatrixWorkspace_sptr createWorkspace(MantidVec& xData, MantidVec& yData, MantidVec& eData,
      const int nSpec = 1)
  {

    Workspace2D_sptr outWS = boost::make_shared<Workspace2D>();
    outWS->initialize(nSpec, xData.size(), yData.size());
    for (int i = 0; i < nSpec; ++i)
    {
      outWS->dataY(i) = yData;
      outWS->dataE(i) = eData;
      outWS->dataX(i) = xData;
    }

    outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

    return outWS;
  }

  MatrixWorkspace_sptr create1DWorkspace(MantidVec& xData, MantidVec& yData)
  {
    Workspace2D_sptr outWS = boost::make_shared<Workspace2D>();
    outWS->initialize(1, xData.size(), yData.size());
    outWS->dataY(0) = yData;
    outWS->dataX(0) = xData;

    outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

    return outWS;
  }

  MatrixWorkspace_sptr a;
  MatrixWorkspace_sptr b;
  MantidVec x;
  MantidVec e;
  typedef boost::tuple<MatrixWorkspace_sptr, double> ResultType;

  MatrixWorkspace_sptr make_arbitrary_point_ws()
  {
    auto x = MantidVec(3);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(3);
    const double ystart = 1;
    const double ystep = 1;
    LinearSequence<MantidVec::value_type> sequenceY(ystart, ystep);
    std::generate(y.begin(), y.end(), sequenceY);

    auto e = MantidVec(3, 1);

    return createWorkspace(x, y, e);
  }

  MatrixWorkspace_sptr make_arbitrary_histogram_ws()
  {
    auto x = MantidVec(3);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(2);
    const double ystart = 1;
    const double ystep = 1;
    LinearSequence<MantidVec::value_type> sequenceY(ystart, ystep);
    std::generate(y.begin(), y.end(), sequenceY);

    auto e = MantidVec(2, 1);

    return createWorkspace(x, y, e);
  }

  MatrixWorkspace_sptr createCosWaveWorkspace(const double startX, const double endX)
  {

    MantidVec xValues;
    MantidVec yValues;
    for (double x = startX; x <= endX; x += 1.0)
    {
      xValues.push_back(x);
      yValues.push_back(std::cos(x));
    }
    yValues.pop_back();

    MatrixWorkspace_sptr outWS = create1DWorkspace(xValues, yValues);

    return outWS;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Stitch1DTest *createSuite()
  {
    return new Stitch1DTest();
  }
  static void destroySuite(Stitch1DTest *suite)
  {
    delete suite;
  }

  Stitch1DTest()
  {

    e = MantidVec(10, 0);
    x = MantidVec(11);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequence(xstart, xstep);
    std::generate(x.begin(), x.end(), sequence);

    MantidVec y = boost::assign::list_of(0)(0)(0)(3)(3)(3)(3)(3)(3)(3).convert_to_container<MantidVec>();

    // Pre-canned workspace to stitch
    a = createWorkspace(x, y, e);

    y = boost::assign::list_of(2)(2)(2)(2)(2)(2)(2)(0)(0)(0).convert_to_container<MantidVec>();
    // Another pre-canned workspace to stitch
    b = createWorkspace(x, y, e);
  }

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs)
  {
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

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs, const MantidVec& params)
  {
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

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs, bool scaleRHS,
      bool useManualScaleFactor, const double& startOverlap, const double& endOverlap,
      const MantidVec& params, const double& manualScaleFactor)
  {
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

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs,
      const double& startOverlap, const double& endOverlap, const MantidVec& params,
      bool scaleRHS = true)
  {
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

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs, const double& overlap,
      const MantidVec& params, const bool startoverlap)
  {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    if (startoverlap)
    {
      alg.setProperty("StartOverlap", overlap);
    }
    else
    {
      alg.setProperty("EndOverlap", overlap);
    }
    alg.setProperty("Params", params);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  void test_Init()
  {
    Stitch1D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize());
    TS_ASSERT( alg.isInitialized());
  }

  void test_startoverlap_greater_than_end_overlap_throws()
  {
    TSM_ASSERT_THROWS("Should have thrown with StartOverlap < x max",
        do_stitch1D(this->a, this->b, this->x.back(), this->x.front(), MantidVec(1, 0.2)),
        std::runtime_error&);
  }

  void test_lhsworkspace_must_be_histogram()
  {
    auto lhs_ws = make_arbitrary_point_ws();
    auto rhs_ws = make_arbitrary_histogram_ws();
    TSM_ASSERT_THROWS("LHS WS must be histogram", do_stitch1D(lhs_ws, rhs_ws, -1, 1, MantidVec(1, 0.2)),
        std::invalid_argument&);
  }

  void test_rhsworkspace_must_be_histogram()
  {
    auto lhs_ws = make_arbitrary_histogram_ws();
    auto rhs_ws = make_arbitrary_point_ws();
    TSM_ASSERT_THROWS("RHS WS must be histogram", do_stitch1D(lhs_ws, rhs_ws, -1, 1, MantidVec(1, 0.2)),
        std::invalid_argument&);
  }

  void test_stitching_uses_suppiled_params()
  {
    MantidVec params = boost::assign::list_of<double>(-0.8)(0.2)(1.0).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4, params);

    MantidVec xValues = ret.get<0>()->readX(0); // Get the output workspace and look at the limits.

    double xMin = xValues.front();
    double xMax = xValues.back();

    TS_ASSERT_EQUALS(xMin, -0.8);
    TS_ASSERT_EQUALS(xMax, 1.0);
  }

  void test_stitching_determines_params()
  {
    MantidVec x1 =
        boost::assign::list_of(-1.0)(-0.8)(-0.6)(-0.4)(-0.2)(0.0)(0.2)(0.4)(0.6)(0.8).convert_to_container<
            MantidVec>();
    MantidVec x2 = boost::assign::list_of(0.4)(0.6)(0.8)(1.0)(1.2)(1.4)(1.6).convert_to_container<
        MantidVec>();
    MantidVec y1 = boost::assign::list_of(1)(1)(1)(1)(1)(1)(1)(1)(1).convert_to_container<MantidVec>();
    MantidVec y2 = boost::assign::list_of(1)(1)(1)(1)(1)(1).convert_to_container<MantidVec>();

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    double demanded_step_size = 0.2;
    auto ret = do_stitch1D(ws1, ws2, 0.4, 1.0,
        boost::assign::list_of(demanded_step_size).convert_to_container<MantidVec>());

    //Check the ranges on the output workspace against the param inputs.
    MantidVec out_x_values = ret.get<0>()->readX(0);
    double x_min = out_x_values.front();
    double x_max = out_x_values.back();
    double step_size = out_x_values[1] - out_x_values[0];

    TS_ASSERT_EQUALS(x_min, -1);
    TS_ASSERT_DELTA(x_max - demanded_step_size, 1.4, 0.000001);
    TS_ASSERT_DELTA(step_size, demanded_step_size, 0.000001);
  }

  void test_stitching_determines_start_and_end_overlap()
  {
    MantidVec x1 =
        boost::assign::list_of(-1.0)(-0.8)(-0.6)(-0.4)(-0.2)(0.0)(0.2)(0.4).convert_to_container<
            MantidVec>();
    MantidVec x2 = boost::assign::list_of(-0.4)(-0.2)(0.0)(0.2)(0.4)(0.6)(0.8)(1.0).convert_to_container<
        MantidVec>();

    MantidVec y1 = boost::assign::list_of(1)(1)(1)(3)(3)(3)(3).convert_to_container<MantidVec>();
    MantidVec y2 = boost::assign::list_of(1)(1)(1)(1)(3)(3)(3).convert_to_container<MantidVec>();

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    MantidVec params = boost::assign::list_of(-1.0)(0.2)(1.0).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(ws1, ws2, params);

    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);

    std::vector<size_t> overlap_indexes = std::vector<size_t>();
    for (size_t itr = 0; itr < stitched_y.size(); ++itr)
    {
      if (stitched_y[itr] >= 1.0009 && stitched_y[itr] <= 3.0001)
      {
        overlap_indexes.push_back(itr);
      }
    }

    double start_overlap_determined = stitched_x[overlap_indexes[0]];
    double end_overlap_determined = stitched_x[overlap_indexes[overlap_indexes.size() - 1]];
    TS_ASSERT_DELTA(start_overlap_determined, -0.4, 0.000000001);
    TS_ASSERT_DELTA(end_overlap_determined, 0.2, 0.000000001);
  }

  void test_stitching_forces_start_overlap()
  {
    MantidVec x1 =
        boost::assign::list_of(-1.0)(-0.8)(-0.6)(-0.4)(-0.2)(0.0)(0.2)(0.4).convert_to_container<
            MantidVec>();
    MantidVec x2 = boost::assign::list_of(-0.4)(-0.2)(0.0)(0.2)(0.4)(0.6)(0.8)(1.0).convert_to_container<
        MantidVec>();

    MantidVec y1 = boost::assign::list_of(1)(1)(1)(3)(3)(3)(3).convert_to_container<MantidVec>();
    MantidVec y2 = boost::assign::list_of(1)(1)(1)(1)(3)(3)(3).convert_to_container<MantidVec>();

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    MantidVec params = boost::assign::list_of(-1.0)(0.2)(1.0).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(ws1, ws2, -0.5, params, true);

    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);

    std::vector<size_t> overlap_indexes = std::vector<size_t>();
    for (size_t itr = 0; itr < stitched_y.size(); ++itr)
    {
      if (stitched_y[itr] >= 1.0009 && stitched_y[itr] <= 3.0001)
      {
        overlap_indexes.push_back(itr);
      }
    }

    double start_overlap_determined = stitched_x[overlap_indexes[0]];
    double end_overlap_determined = stitched_x[overlap_indexes[overlap_indexes.size() - 1]];
    TS_ASSERT_DELTA(start_overlap_determined, -0.4, 0.000000001);
    TS_ASSERT_DELTA(end_overlap_determined, 0.2, 0.000000001);
  }

  void test_stitching_forces_end_overlap()
  {
    MantidVec x1 =
        boost::assign::list_of(-1.0)(-0.8)(-0.6)(-0.4)(-0.2)(0.0)(0.2)(0.4).convert_to_container<
            MantidVec>();
    MantidVec x2 = boost::assign::list_of(-0.4)(-0.2)(0.0)(0.2)(0.4)(0.6)(0.8)(1.0).convert_to_container<
        MantidVec>();

    MantidVec y1 = boost::assign::list_of(1)(1)(1)(3)(3)(3)(3).convert_to_container<MantidVec>();
    MantidVec y2 = boost::assign::list_of(1)(1)(1)(1)(3)(3)(3).convert_to_container<MantidVec>();

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, y1);
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, y2);
    MantidVec params = boost::assign::list_of(-1.0)(0.2)(1.0).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(ws1, ws2, 0.5, params, false);

    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);

    std::vector<size_t> overlap_indexes = std::vector<size_t>();
    for (size_t itr = 0; itr < stitched_y.size(); ++itr)
    {
      if (stitched_y[itr] >= 1.0009 && stitched_y[itr] <= 3.0001)
      {
        overlap_indexes.push_back(itr);
      }
    }

    double start_overlap_determined = stitched_x[overlap_indexes[0]];
    double end_overlap_determined = stitched_x[overlap_indexes[overlap_indexes.size() - 1]];
    TS_ASSERT_DELTA(start_overlap_determined, -0.4, 0.000000001);
    TS_ASSERT_DELTA(end_overlap_determined, 0.2, 0.000000001);
  }

  void test_stitching_scale_right()
  {
    MantidVec params = boost::assign::list_of<double>(0.2).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4, params);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 2.0/3.0, 0.000000001);
    // Fetch the arrays from the output workspace
    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);
    MantidVec stitched_e = ret.get<0>()->readE(0);
    // Check that the output Y-Values are correct. Output Y Values should all be 2
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr)
    {
      TS_ASSERT_DELTA(2, *itr, 0.000001);
    }
    // Check that the output E-Values are correct. Output Error values should all be zero
    for (auto itr = stitched_e.begin(); itr != stitched_e.end(); ++itr)
    {
      double temp = *itr;
      TS_ASSERT_EQUALS(temp, 0);
    }
    // Check that the output X-Values are correct.
    //truncate the input and oputput x values to 6 decimal places to eliminate insignificant error
    MantidVec xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(), roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x);
  }

  void test_stitching_scale_left()
  {
    MantidVec params = boost::assign::list_of<double>(0.2).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4, params, false);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 3.0/2.0, 0.000000001);
    // Fetch the arrays from the output workspace
    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);
    MantidVec stitched_e = ret.get<0>()->readE(0);
    // Check that the output Y-Values are correct. Output Y Values should all be 3
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr)
    {
      TS_ASSERT_DELTA(3, *itr, 0.000001);
    }
    // Check that the output E-Values are correct. Output Error values should all be zero
    for (auto itr = stitched_e.begin(); itr != stitched_e.end(); ++itr)
    {
      double temp = *itr;
      TS_ASSERT_EQUALS(temp, 0);
    }
    // Check that the output X-Values are correct.
    //truncate the input and oputput x values to 6 decimal places to eliminate insignificant error
    MantidVec xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(), roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x);
  }

  void test_stitching_manual_scale_factor_scale_right()
  {
    MantidVec params = boost::assign::list_of<double>(0.2).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(this->b, this->a, true, true, -0.4, 0.4, params, 2.0 / 3.0);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 2.0/3.0, 0.000000001);
    // Fetch the arrays from the output workspace
    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);
    MantidVec stitched_e = ret.get<0>()->readE(0);
    // Check that the output Y-Values are correct. Output Y Values should all be 2
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr)
    {
      TS_ASSERT_DELTA(2, *itr, 0.000001);
    }
    // Check that the output E-Values are correct. Output Error values should all be zero
    for (auto itr = stitched_e.begin(); itr != stitched_e.end(); ++itr)
    {
      double temp = *itr;
      TS_ASSERT_EQUALS(temp, 0);
    }
    // Check that the output X-Values are correct.
    //truncate the input and oputput x values to 6 decimal places to eliminate insignificant error
    MantidVec xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(), roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x);
  }

  void test_stitching_manual_scale_factor_scale_left()
  {
    MantidVec params = boost::assign::list_of<double>(0.2).convert_to_container<MantidVec>();
    auto ret = do_stitch1D(this->b, this->a, false, true, -0.4, 0.4, params, 3.0 / 2.0);

    double scale = ret.get<1>();
    // Check the scale factor
    TS_ASSERT_DELTA(scale, 3.0/2.0, 0.000000001);
    // Fetch the arrays from the output workspace
    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);
    MantidVec stitched_e = ret.get<0>()->readE(0);
    // Check that the output Y-Values are correct. Output Y Values should all be 3
    for (auto itr = stitched_y.begin(); itr != stitched_y.end(); ++itr)
    {
      TS_ASSERT_DELTA(3, *itr, 0.000001);
    }
    // Check that the output E-Values are correct. Output Error values should all be zero
    for (auto itr = stitched_e.begin(); itr != stitched_e.end(); ++itr)
    {
      double temp = *itr;
      TS_ASSERT_EQUALS(temp, 0);
    }
    // Check that the output X-Values are correct.
    //truncate the input and oputput x values to 6 decimal places to eliminate insignificant error
    MantidVec xCopy = this->x;
    std::transform(stitched_x.begin(), stitched_x.end(), stitched_x.begin(), roundSix);
    std::transform(xCopy.begin(), xCopy.end(), xCopy.begin(), roundSix);
    TS_ASSERT(xCopy == stitched_x);
  }

  void test_params_causing_scaling_regression_test()
  {
    auto lhs = createCosWaveWorkspace(0, 10);
    auto rhs = createCosWaveWorkspace(6, 20);

    auto ret = do_stitch1D(lhs, rhs);

    MatrixWorkspace_sptr outWS = ret.get<0>();
    const double scaleFactor = ret.get<1>();

    TSM_ASSERT_EQUALS("Two cosine waves in phase scale factor should be unity", 1.0, scaleFactor);
    const double stitchedWSFirstYValue = outWS->readY(0)[0]; // Should be 1.0 at cos(0)
    const double lhsWSFirstYValue = lhs->readY(0)[0]; // Should be 1.0 at cos(0)

    TSM_ASSERT_EQUALS("No scaling of the output workspace should have occurred", stitchedWSFirstYValue,
        lhsWSFirstYValue);
  }

  void test_has_non_zero_errors_single_spectrum()
  {
    auto x = MantidVec(10);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(x.size() - 1, 1);

    auto e = MantidVec(x.size() - 1, 1); // All Non zero errors

    MatrixWorkspace_sptr ws = createWorkspace(x, y, e, 1);
    Stitch1D alg;
    TSM_ASSERT("All error values are non-zero", alg.hasNonzeroErrors(ws));

    // Run it again with all zeros
    e = MantidVec(x.size() - 1, 0); // All zero errors
    ws = createWorkspace(x, y, e, 1);
    TSM_ASSERT("All error values are non-zero", !alg.hasNonzeroErrors(ws));

    // Run it again with some zeros
    e[e.size() - 1] = 1;
    ws = createWorkspace(x, y, e, 1);
    TSM_ASSERT("NOT all error values are non-zero", alg.hasNonzeroErrors(ws));
  }

  void test_has_non_zero_errors_multiple_spectrum()
  {
    const size_t nspectrum = 10;

    auto x = MantidVec(10);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(nspectrum * (x.size() - 1), 1);
    auto e = MantidVec(nspectrum * (x.size() - 1), 1); // Non zero errors

    MatrixWorkspace_sptr ws = createWorkspace(x, y, e, static_cast<int>(nspectrum));
    Stitch1D alg;
    TSM_ASSERT("All error values are non-zero", alg.hasNonzeroErrors(ws));

    // Run it again with all zeros
    e = MantidVec(nspectrum * (x.size() - 1), 0); // All zero errors
    ws = createWorkspace(x, y, e, nspectrum);
    TSM_ASSERT("All error values are non-zero", !alg.hasNonzeroErrors(ws));

    // Run it again with some zeros
    e[e.size() - 1] = 1;
    ws = createWorkspace(x, y, e, nspectrum);
    TSM_ASSERT("NOT all error values are non-zero", alg.hasNonzeroErrors(ws));
  }

  void test_patch_nan_y_value_for_scaling()
  {
    const size_t nspectrum = 1;

    auto x = MantidVec(10);
    const double xstart = 0;
    const double xstep = 1;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(nspectrum * (x.size() - 1), 1);
    auto e = MantidVec(nspectrum * (x.size() - 1), 1);

    double nan = std::numeric_limits<double>::quiet_NaN();
    y[5] = nan; // Add a NAN
    MatrixWorkspace_sptr lhsWS = createWorkspace(x, y, e, static_cast<int>(nspectrum));

    y[5] = y[4];
    // Remove NAN
    MatrixWorkspace_sptr rhsWS = createWorkspace(x, y, e, static_cast<int>(nspectrum));

    auto ret = do_stitch1D(lhsWS, rhsWS);

    double scaleFactor = ret.get<1>();

    TSM_ASSERT("ScaleFactor should not be NAN", !boost::math::isnan(scaleFactor));
  }

  void test_patch_inf_y_value_for_scaling()
  {
    const size_t nspectrum = 1;

    auto x = MantidVec(10);
    const double xstart = 0;
    const double xstep = 1;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(nspectrum * (x.size() - 1), 1);
    auto e = MantidVec(nspectrum * (x.size() - 1), 1);

    double inf = std::numeric_limits<double>::infinity();
    y[5] = inf; // Add a Infinity
    MatrixWorkspace_sptr lhsWS = createWorkspace(x, y, e, static_cast<int>(nspectrum));

    y[5] = y[4];
    // Remove infinity
    MatrixWorkspace_sptr rhsWS = createWorkspace(x, y, e, static_cast<int>(nspectrum));

    auto ret = do_stitch1D(lhsWS, rhsWS);

    double scaleFactor = ret.get<1>();

    TSM_ASSERT("ScaleFactor should not be Infinity", !boost::math::isinf(scaleFactor));
  }


  void test_reset_nans()
  {
    const size_t nspectrum = 1;

    auto x = MantidVec(10);
    const double xstart = 0;
    const double xstep = 1;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(nspectrum * (x.size() - 1), 1);
    auto e = MantidVec(nspectrum * (x.size() - 1), 1);

    double nan = std::numeric_limits<double>::quiet_NaN();
    y[0] = nan; // Add a Infinity
    MatrixWorkspace_sptr lhsWS = createWorkspace(x, y, e, static_cast<int>(nspectrum));

    y[0] = y[1];
    // Remove infinity
    MatrixWorkspace_sptr rhsWS = createWorkspace(x, y, e, static_cast<int>(nspectrum));

    auto ret = do_stitch1D(lhsWS, rhsWS);

    MatrixWorkspace_sptr outWs = ret.get<0>();
    double scaleFactor = ret.get<1>();

    TSM_ASSERT("ScaleFactor should not be Infinity", !boost::math::isinf(scaleFactor));

    auto outY = outWs->readY(0);
    TSM_ASSERT("Nans should be put back", boost::math::isnan(outY[0]));
  }


};

#endif /* MANTID_ALGORITHMS_STITCH1DTEST_H_ */
