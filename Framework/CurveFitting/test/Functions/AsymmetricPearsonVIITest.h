// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <math.h>
#include <memory>

#include "MantidAPI/PeakFunctionIntegrator.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/Functions/AsymmetricPearsonVII.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Lorentzian.h"
#include "MantidCurveFitting/Jacobian.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::CostFunctions;

class AsymmetricPearsonVIITest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AsymmetricPearsonVIITest *createSuite() { return new AsymmetricPearsonVIITest(); }
  static void destroySuite(AsymmetricPearsonVIITest *suite) { delete suite; }

  AsymmetricPearsonVIITest() : m_xValues(), m_10xValues() {
    for (size_t i = 0; i < 2000; ++i) {
      double x_i = -1 + 0.001 * static_cast<double>(i);
      m_xValues.emplace_back(x_i);
    }

    for (size_t i = 0; i < 10; ++i) {
      double x_i = -1 + 0.2 * static_cast<double>(i);
      m_10xValues.emplace_back(x_i);
    }
  }

  void testCategory() {
    AsymmetricPearsonVII ap7;
    TS_ASSERT_EQUALS(ap7.category(), "XrayDiffraction");
  }

  void testParametersInitialization() {
    AsymmetricPearsonVII ap7;
    ap7.initialize();
    // test initialized values
    TS_ASSERT_DELTA(ap7.height(), 1.0, 1.0e-6);
    TS_ASSERT_DELTA(ap7.centre(), 0.0, 1.0e-6);
    TS_ASSERT_DELTA(ap7.fwhm(), 0.1, 1.0e-6);
    TS_ASSERT_DELTA(ap7.leftShape(), 1.0, 1.0e-6);
    TS_ASSERT_DELTA(ap7.rightShape(), 1.0, 1.0e-6);
  }

  void testSetParameters() {
    AsymmetricPearsonVII ap7;
    const double ph = 0.9;
    const double pc = 0.5;
    const double width = 1.1;
    const double ml = 2.0;
    const double mr = 0.0;
    ap7.initialize();

    ap7.setHeight(ph);
    ap7.setCentre(pc);
    ap7.setFwhm(width);
    ap7.setLeftShape(ml);
    ap7.setRightShape(mr);
    // test change of the values
    TS_ASSERT_DELTA(ap7.height(), ph, 1.0e-6);
    TS_ASSERT_DELTA(ap7.centre(), pc, 1.0e-6);
    TS_ASSERT_DELTA(ap7.fwhm(), width, 1.0e-6);
    TS_ASSERT_DELTA(ap7.leftShape(), ml, 1.0e-6);
    TS_ASSERT_DELTA(ap7.rightShape(), mr, 1.0e-6);
  }

  /** Test expected output
   * @brief testExpectedOutput: test if the asymmetric Pearson VII
   * function returns the expected values on the interval [-1, 1)
   * and step 0.2
   */
  void testExpectedOutput() {
    const double ph = 10.0;
    const double pc = 0.0;
    const double width = 0.1;
    const double ml = 1.75;
    const double mr = 1.0;

    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, width, ml, mr);
    FunctionDomain1DVector domain(m_10xValues);
    FunctionValues values_ap7(domain);
    ap7->function(domain, values_ap7);

    double expected_output[10] = {0.000979244, 0.00212761, 0.00576074, 0.0230956, 0.22348,
                                  10.,         0.588235,   0.153846,   0.0689655, 0.0389105};

    for (size_t i = 0; i < values_ap7.size(); ++i) {
      TS_ASSERT_DELTA(expected_output[i], values_ap7[i], 1e-6);
    }
  }

  /** Test integral
   * @brief testIntegral: test if the integral of the asymmetric Pearson VII
   * function over the interval (-100, 100) returns the expected value
   */
  void testIntegral() {
    const double ph = 100.0;
    const double pc = 0.0;
    const double width = 0.7;
    const double ml = 1.7;
    const double mr = 2.0;
    const double ll = -100.0;
    const double ul = 100.0;
    const double expected_value = 86.8874;

    double num_integral = numerical_integrate_ap7(ph, pc, width, ml, mr, ll, ul);
    TS_ASSERT_DELTA(num_integral, expected_value, 1.0e-4);
  }

  /** Test asymptotic behavior
   * @brief testAsymptoticBehaviorLimitmEq1: test if the
   * asymmetric Pearson VII behaves as Lorentzian at m = 1
   */
  void testAsymptoticBehaviorLimitmEq1() {
    const double ph = 10.0;
    const double pc = 0.0;
    const double width = 0.7;

    // initialize AsymmetricPearsonVII with ml=mr=1
    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, width, 1.0, 1.0);
    FunctionDomain1DVector domain(m_xValues);
    FunctionValues values_ap7(domain);
    ap7->function(domain, values_ap7);

    // initialize Lorentzian
    Lorentzian lr;
    lr.initialize();
    lr.setCentre(pc);
    lr.setHeight(ph);
    lr.setFwhm(width);
    FunctionValues values_lorentzian(domain);
    lr.function(domain, values_lorentzian);

    // compare AsymmetricPearsonVII with Lorentzian
    for (size_t i = 0; i < values_lorentzian.size(); ++i)
      TS_ASSERT_DELTA(values_lorentzian[i], values_ap7[i], 1.0e-6);
  }

  /** Test asymptotic behavior
   * @brief testAsymptoticBehaviorLimitmEqInfty: test if the
   * asymmetric Pearson VII behaves as Gaussian at m -> \infty
   */
  void testAsymptoticBehaviorLimitmEqInfty() {
    const double ph = 210.0;
    const double pc = 3.0;
    const double width = 12.7;

    // initialize AsymmetricPearsonVII with ml=mr=10^8
    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, width, 1.0e8, 1.0e8);
    FunctionDomain1DVector domain(m_xValues);
    FunctionValues values_ap7(domain);
    ap7->function(domain, values_ap7);

    // initialize Gaussian
    Gaussian gaus;
    gaus.initialize();
    gaus.setCentre(pc);
    gaus.setHeight(ph);
    gaus.setFwhm(width);
    FunctionValues values_gaussian(domain);
    gaus.function(domain, values_gaussian);

    // compare AsymmetricPearsonVII with Gaussian
    for (size_t i = 0; i < values_gaussian.size(); ++i)
      TS_ASSERT_DELTA(values_gaussian[i], values_ap7[i], 1.0e-4);
  }

  /** Test derivative w.r.t. peak height
   * @brief testDerivativeVaryingHeight: test partial derivative of the asymmetric
   * Pearson VII with respect to the peak height
   */
  void testDerivativeVaryingHeight() {
    const double min_ph = 0.9;
    const double max_ph = 1.1;
    const double pc = -1.0;
    const double width = 4.0;
    const double ml = 1.7;
    const double mr = 10.0;
    const double ph_resolution = 0.005;

    // create function
    IPeakFunction_sptr ap7 = getInitializedAPVII(min_ph, pc, width, ml, mr);

    // evalulate at N points
    std::vector<double> xValues{-1};
    for (double x : xValues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 5);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_ph;
      while (param_value < max_ph - ph_resolution) {
        // update parameter and calcualte Jocobian
        ap7->setParameter(0, param_value);
        ap7->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.emplace_back(jacobian.get(0, 0));
        param_value += ph_resolution;
      }

      // calculate numerically
      std::vector<double> vec_ph;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(ap7, 0, min_ph, max_ph, ph_resolution, x, vec_ph, vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_ph.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1.0e-6);
      }
    }
    return;
  }

  /** Test derivative w.r.t. peak centre
   * @brief testDerivativeVaryingCentre: test partial derivative of the asymmetric
   * Pearson VII with respect to the peak centre
   */
  void testDerivativeVaryingCentre() {
    const double ph = 2.0;
    const double min_pc = -1.0;
    const double max_pc = 0.5;
    const double width = 3.5;
    const double ml = 2.7;
    const double mr = 5.0;
    double pc_resolution = 0.005;

    // create function
    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, min_pc, width, ml, mr);

    // evalulate at N points
    std::vector<double> xValues{-1};
    for (double x : xValues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 5);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_pc;
      while (param_value < max_pc - pc_resolution) {
        // update parameter and calcualte Jocobian
        ap7->setParameter(1, param_value);
        ap7->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.emplace_back(jacobian.get(0, 1));
        param_value += pc_resolution;
      }

      // calculate numerically
      std::vector<double> vec_pc;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(ap7, 1, min_pc, max_pc, pc_resolution, x, vec_pc, vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_pc.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1.0e-2);
      }
    }
    return;
  }

  /** Test derivative w.r.t. width
   * @brief testDerivativeVaryingFWHM: test partial derivative of the asymmetric
   * Pearson VII with respect to the fwhm
   */
  void testDerivativeVaryingFWHM() {
    const double ph = 2;
    const double pc = 0.0;
    const double min_width = 3.5;
    const double max_width = 4.5;
    const double ml = 2.7;
    const double mr = 5.0;
    double width_resolution = 0.005;

    // create function
    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, min_width, ml, mr);

    // evalulate at N points
    std::vector<double> xValues{0};
    for (double x : xValues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 5);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_width;
      while (param_value < max_width - width_resolution) {
        // update parameter and calcualte Jocobian
        ap7->setParameter(2, param_value);
        ap7->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.emplace_back(jacobian.get(0, 2));
        param_value += width_resolution;
      }

      // calculate numerically
      std::vector<double> vec_width;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(ap7, 2, min_width, max_width, width_resolution, x, vec_width,
                                         vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_width.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1.0e-2);
      }
    }
    return;
  }

  /** Test derivative w.r.t. left shape
   * @brief testDerivativeVaryingLeftShape: test partial derivative of the asymmetric
   * Pearson VII with respect to the left shape parameter
   */
  void testDerivativeVaryingLeftShape() {
    const double ph = 20.0;
    const double pc = 1.2;
    const double width = 3.5;
    const double min_ml = 0.7;
    const double max_ml = 2.7;
    const double mr = 5.0;
    const double ml_resolution = 0.005;

    // create function
    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, width, min_ml, mr);

    // evalulate at N points
    std::vector<double> xValues{1.2};
    for (double x : xValues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 5);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_ml;
      while (param_value < max_ml - ml_resolution) {
        // update parameter and calcualte Jocobian
        ap7->setParameter(3, param_value);
        ap7->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.emplace_back(jacobian.get(0, 3));
        param_value += ml_resolution;
      }

      // calculate numerically
      std::vector<double> vec_ml;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(ap7, 3, min_ml, max_ml, ml_resolution, x, vec_ml, vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_ml.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1e-2);
      }
    }
    return;
  }

  /** Test derivative w.r.t. right shape
   * @brief testDerivativeVaryingRightShape: test partial derivative of the asymmetric
   * Pearson VII with respect to the right shape parameter
   */
  void testDerivativeVaryingRightShape() {
    const double ph = 20.0;
    const double pc = -1.0;
    const double width = 3.5;
    const double ml = 1.0;
    const double min_mr = 0.1;
    const double max_mr = 10;
    const double mr_resolution = 0.005;

    // create function
    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, width, ml, min_mr);

    // evalulate at N points
    std::vector<double> xValues{-1};
    for (double x : xValues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 5);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_mr;
      while (param_value < max_mr - mr_resolution) {
        // update parameter and calcualte Jocobian
        ap7->setParameter(4, param_value);
        ap7->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.emplace_back(jacobian.get(0, 4));
        param_value += mr_resolution;
      }

      // calculate numerically
      std::vector<double> vec_mr;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(ap7, 4, min_mr, max_mr, mr_resolution, x, vec_mr, vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_mr.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1.0e-2);
      }
    }
    return;
  }

  /** Test limit ml-->0
   * @brief testLeftShapeLimit: test if the left shape limit of the
   * asymmetric Pearson VII function as ml->0 is properly evaluated
   */
  void testLeftShapeLimit() {
    const double ph = 100.0;
    const double pc = 0.0;
    const double width = 0.7;
    const double ml = 0.0;
    const double mr = 2.0;
    const double expected_pred = ph / 2.0;

    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, width, ml, mr);
    std::vector<double> xValues{-1.2};
    FunctionDomain1DVector domain(xValues);
    FunctionValues values_ap7(domain);
    ap7->function(domain, values_ap7);

    // check asymmetric Pearson VII at ml = 0
    TS_ASSERT_DELTA(expected_pred, values_ap7[0], 1e-4);

    Mantid::CurveFitting::Jacobian jacobian(domain.size(), 5);
    std::vector<double> vec_deriv_pred;
    double expected_deriv_pred[5] = {0.5, 0.0, 0.0, -123.2144, 0.0};
    ap7->functionDeriv(domain, jacobian);

    // check derivatives at ml = 0 and x = -1.2
    for (size_t i = 0; i < 5; ++i) {
      vec_deriv_pred.emplace_back(jacobian.get(0, i));
      TS_ASSERT_DELTA(expected_deriv_pred[i], vec_deriv_pred[i], 1e-4);
    }
  }

  /** Test limit mr-->0
   * @brief testRightShapeLimit: test if the right shape limit of the
   * asymmetric Pearson VII function as mr->0 is properly evaluated
   */
  void testRightShapeLimit() {
    const double ph = 1.0;
    const double pc = 10.0;
    const double width = 1.7;
    const double ml = 0.2;
    const double mr = 0.0;
    const double expected_pred = ph / 2.0;

    IPeakFunction_sptr ap7 = getInitializedAPVII(ph, pc, width, ml, mr);
    std::vector<double> xValues{12};
    FunctionDomain1DVector domain(xValues);
    FunctionValues values_ap7(domain);
    ap7->function(domain, values_ap7);

    // check asymmetric Pearson VII at mr = 0
    TS_ASSERT_DELTA(expected_pred, values_ap7[0], 1e-4);

    Mantid::CurveFitting::Jacobian jacobian(domain.size(), 5);
    std::vector<double> vec_deriv_pred;
    double expected_deriv_pred[5] = {0.5, 0.0, 0.0, 0.0, -0.855666};
    ap7->functionDeriv(domain, jacobian);

    // check derivatives at mr = 0 and x = 12
    for (size_t i = 0; i < 5; ++i) {
      vec_deriv_pred.emplace_back(jacobian.get(0, i));
      TS_ASSERT_DELTA(expected_deriv_pred[i], vec_deriv_pred[i], 1e-4);
    }
  }

  void test_with_Levenberg_Marquardt() {
    // right pearson VII is non-zero
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(79292.4, 79603.6, 100));
    API::FunctionValues mockData(*domain);
    AsymmetricPearsonVII dataMaker;

    const double ph_dm = 250.0;
    const double pc_dm = 79450.0;
    const double width_dm = 30.0;
    const double ml_dm = 1.0;
    const double mr_dm = 1.0;

    dataMaker.initialize();
    dataMaker.setHeight(ph_dm);
    dataMaker.setCentre(pc_dm);
    dataMaker.setFwhm(width_dm);
    dataMaker.setLeftShape(ml_dm);
    dataMaker.setRightShape(mr_dm);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    const double ph = 232.11;
    const double pc = 79430.1;
    const double width = 26.14;
    const double ml = 10.0;
    const double mr = 2.0;

    // set up asymmetric Pearson VII fitting function
    std::shared_ptr<AsymmetricPearsonVII> ap7 = std::make_shared<AsymmetricPearsonVII>();
    ap7->initialize();
    ap7->setParameter("PeakHeight", ph);
    ap7->setParameter("PeakCentre", pc);
    ap7->setParameter("Width", width);
    ap7->setParameter("LeftShape", ml);
    ap7->setParameter("RightShape", mr);

    std::shared_ptr<CostFuncLeastSquares> costFun = std::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(ap7, domain, values);

    FuncMinimisers::LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    API::IFunction_sptr res = costFun->getFittingFunction();
  }

private:
  IPeakFunction_sptr getInitializedAPVII(double ph, double pc, double width, double ml, double mr) {
    IPeakFunction_sptr ap7 = std::make_shared<AsymmetricPearsonVII>();
    ap7->initialize();
    ap7->setParameter("PeakHeight", ph);
    ap7->setParameter("PeakCentre", pc);
    ap7->setParameter("Width", width);
    ap7->setParameter("LeftShape", ml);
    ap7->setParameter("RightShape", mr);

    return ap7;
  }

  double numerical_integrate_ap7(double ph, double pc, double width, double ml, double mr, double lower_lim,
                                 double upper_lim) {
    AsymmetricPearsonVII ap7;
    ap7.initialize();
    ap7.setParameter(0, ph);
    ap7.setParameter(1, pc);
    ap7.setParameter(2, width);
    ap7.setParameter(3, ml);
    ap7.setParameter(4, mr);

    PeakFunctionIntegrator integrator;
    IntegrationResult result = integrator.integrate(ap7, lower_lim, upper_lim);

    return result.result;
  }

  /** evalulate the derivative numerically for an arbitrary parameter at x
   * calculate \partial pV() / \partial p_i  where i = 0, 1, 2, 3, 4.
   * @param ap7 : IPeakFunction pointer to AsymmetricPearsonVII
   * @param param_index: index of the parameter (peak hight: 0; peak centre: 1;
   * fwhm: 2; left shape parameter: 3; right shape parameter: 4)
   * @param min_value: minimum value of the parameter to evalulate the
   * derivative
   * @param max_value: maximum value of the parameter to evalulate the
   * derivative
   * @param resolution: resolution (min step)
   * @param x:x value
   * @param param_vec: (output) parameter value vector
   * @param deriv_vec: (output) derivative vector
   */
  void numerical_param_partial_derivative(IPeakFunction_sptr &ap7, size_t param_index, double min_value,
                                          double max_value, double resolution, double x, std::vector<double> &param_vec,
                                          std::vector<double> &deriv_vec) {
    // create a single value vector for domain
    std::vector<double> vec_x{x};
    FunctionDomain1DVector domain(vec_x);
    FunctionValues values(domain);

    // calculate pv value of a given X with changing parameter value
    double param_value = min_value - resolution;
    param_vec.clear();
    std::vector<double> ap7_vec;
    while (param_value < max_value - resolution) {
      // update parameter and calculate
      ap7->setParameter(param_index, param_value);
      ap7->function(domain, values);
      // set to vector
      param_vec.emplace_back(param_value);
      ap7_vec.emplace_back(values[0]);
      // increment
      param_value += resolution;
    }

    // evalulate derivative to parameter (single way dx = (f(x+h) - f(x))/h
    deriv_vec.resize(param_vec.size() - 1);
    for (size_t i = 0; i < param_vec.size() - 1; ++i) {
      deriv_vec[i] = (ap7_vec[i + 1] - ap7_vec[i]) / resolution;
    }
    // pop out the last element of parameter vector
    param_vec.pop_back();

    return;
  }

  std::vector<double> m_xValues;
  std::vector<double> m_10xValues;
};
