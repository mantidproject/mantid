// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_PSEUDOVOIGTTEST_H_
#define MANTID_CURVEFITTING_PSEUDOVOIGTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/PeakFunctionIntegrator.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Lorentzian.h"
#include "MantidCurveFitting/Functions/PseudoVoigt.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidKernel/MersenneTwister.h"

#include <boost/make_shared.hpp>

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class PseudoVoigtTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PseudoVoigtTest *createSuite() { return new PseudoVoigtTest(); }
  static void destroySuite(PseudoVoigtTest *suite) { delete suite; }

  PseudoVoigtTest()
      : m_xValues(), m_yValues(), m_dfdh(), m_dfda(), m_dfdx0(), m_dfdf() {
    for (size_t i = 0; i < 200; ++i) {
      double x_i = -10 + 0.1 * static_cast<double>(i);
      m_xValues.push_back(x_i);
    }

    m_yValues.push_back(4.372997125267132);
    m_yValues.push_back(4.458629118465070);
    m_yValues.push_back(4.535563492585204);
    m_yValues.push_back(4.603064037523992);
    m_yValues.push_back(4.660455187114265);
    m_yValues.push_back(4.707139614264023);
    m_yValues.push_back(4.742615179498014);
    m_yValues.push_back(4.766490204566635);
    m_yValues.push_back(4.778496044066421);
    m_yValues.push_back(4.778496044066421);
    m_yValues.push_back(4.766490204566637);
    m_yValues.push_back(4.742615179498014);
    m_yValues.push_back(4.707139614264019);
    m_yValues.push_back(4.660455187114265);
    m_yValues.push_back(4.603064037523992);
    m_yValues.push_back(4.535563492585212);
    m_yValues.push_back(4.458629118465070);
    m_yValues.push_back(4.372997125267132);
    m_yValues.push_back(4.279447055100300);
    m_yValues.push_back(4.178785512380577);
    m_yValues.push_back(4.071831485496261);

    m_dfdh.push_back(0.914852955076807);
    m_dfdh.push_back(0.932767598005245);
    m_dfdh.push_back(0.948862655352554);
    m_dfdh.push_back(0.962984108268618);
    m_dfdh.push_back(0.974990624919302);
    m_dfdh.push_back(0.984757241477829);
    m_dfdh.push_back(0.992178907844773);
    m_dfdh.push_back(0.997173682963731);
    m_dfdh.push_back(0.999685364867452);
    m_dfdh.push_back(0.999685364867452);
    m_dfdh.push_back(0.997173682963731);
    m_dfdh.push_back(0.992178907844773);
    m_dfdh.push_back(0.984757241477829);
    m_dfdh.push_back(0.974990624919302);
    m_dfdh.push_back(0.962984108268618);
    m_dfdh.push_back(0.948862655352554);
    m_dfdh.push_back(0.932767598005245);
    m_dfdh.push_back(0.914852955076807);
    m_dfdh.push_back(0.895281810690438);
    m_dfdh.push_back(0.874222910539870);
    m_dfdh.push_back(0.851847591108002);

    m_dfda.push_back(0.127423417613684);
    m_dfda.push_back(0.105761666867053);
    m_dfda.push_back(0.083998491075912);
    m_dfda.push_back(0.063081569151440);
    m_dfda.push_back(0.043939766110092);
    m_dfda.push_back(0.027438762645369);
    m_dfda.push_back(0.014336810534878);
    m_dfda.push_back(0.005243855136706);
    m_dfda.push_back(0.000587294644077);
    m_dfda.push_back(0.000587294644077);
    m_dfda.push_back(0.005243855136706);
    m_dfda.push_back(0.014336810534878);
    m_dfda.push_back(0.027438762645369);
    m_dfda.push_back(0.043939766110092);
    m_dfda.push_back(0.063081569151440);
    m_dfda.push_back(0.083998491075912);
    m_dfda.push_back(0.105761666867053);
    m_dfda.push_back(0.127423417613684);
    m_dfda.push_back(0.148058862985728);
    m_dfda.push_back(0.166802486088368);
    m_dfda.push_back(0.182878080915878);

    m_dfdx0.push_back(-8.963400576569903e+01);
    m_dfdx0.push_back(-8.132865068366561e+01);
    m_dfdx0.push_back(-7.226335976168113e+01);
    m_dfdx0.push_back(-6.248995205947752e+01);
    m_dfdx0.push_back(-5.207782518137794e+01);
    m_dfdx0.push_back(-4.111379724585275e+01);
    m_dfdx0.push_back(-2.970095613292614e+01);
    m_dfdx0.push_back(-1.795646367180882e+01);
    m_dfdx0.push_back(-6.008372247750958e+00);
    m_dfdx0.push_back(6.008372247750958e+00);
    m_dfdx0.push_back(1.795646367180882e+01);
    m_dfdx0.push_back(2.970095613292614e+01);
    m_dfdx0.push_back(4.111379724585275e+01);
    m_dfdx0.push_back(5.207782518137794e+01);
    m_dfdx0.push_back(6.248995205947752e+01);
    m_dfdx0.push_back(7.226335976168113e+01);
    m_dfdx0.push_back(8.132865068366561e+01);
    m_dfdx0.push_back(8.963400576569903e+01);
    m_dfdx0.push_back(9.714448961626630e+01);
    m_dfdx0.push_back(1.038406984991238e+02);
    m_dfdx0.push_back(1.097169693748341e+02);

    m_dfdf.push_back(1.525303401418302e+01);
    m_dfdf.push_back(1.221150911166150e+01);
    m_dfdf.push_back(9.403640409427975e+00);
    m_dfdf.push_back(6.880775502044572e+00);
    m_dfdf.push_back(4.691695962286301e+00);
    m_dfdf.push_back(2.880846653863556e+00);
    m_dfdf.push_back(1.486534340987295e+00);
    m_dfdf.push_back(5.392331432975621e-01);
    m_dfdf.push_back(6.014386634385344e-02);
    m_dfdf.push_back(6.014386634385344e-02);
    m_dfdf.push_back(5.392331432975621e-01);
    m_dfdf.push_back(1.486534340987295e+00);
    m_dfdf.push_back(2.880846653863556e+00);
    m_dfdf.push_back(4.691695962286301e+00);
    m_dfdf.push_back(6.880775502044572e+00);
    m_dfdf.push_back(9.403640409427975e+00);
    m_dfdf.push_back(1.221150911166150e+01);
    m_dfdf.push_back(1.525303401418302e+01);
    m_dfdf.push_back(1.847592895604664e+01);
    m_dfdf.push_back(2.182837505987588e+01);
    m_dfdf.push_back(2.526016311933117e+01);
  }

  void testCategory() {
    PseudoVoigt fn;
    TS_ASSERT_EQUALS(fn.category(), "Peak");
  }

  /** test setParameter such that H, I, eta and peak height shall be related
   */
  void testSetParameters() {
    // create a Gaussian
    Gaussian gaussian;
    gaussian.initialize();
    gaussian.setFwhm(0.5);
    gaussian.setHeight(2.0);
    double intensity = gaussian.intensity();

    PseudoVoigt pv;
    pv.initialize();

    // set PV as a Gaussian and test inexplicitly calculating intensity
    pv.setParameter("Mixing", 1.);
    pv.setFwhm(0.5);
    pv.setHeight(2.0);
    double pv_intensity = pv.intensity();
    TS_ASSERT_DELTA(intensity, pv_intensity, 1.E-4);

    // change mixing to Lorentzian
    Lorentzian lr;
    lr.initialize();
    lr.setIntensity(pv_intensity);
    lr.setFwhm(0.5);
    double lr_height = lr.height();

    pv.setHeight(lr_height);
    pv.setIntensity(lr.intensity());
    double lr_mixing = pv.getParameter("Mixing");
    TS_ASSERT_DELTA(lr_mixing, 0., 1E-5);

    // set intensity again to modify the peak width
    pv.setParameter("Mixing", lr_mixing); // accept current mixing
    pv.setIntensity(2 * pv_intensity);
    double pv_fwhm = pv.fwhm();
    TS_ASSERT_DELTA(pv_fwhm, 1.0, 1.E-5);

    // increase height again to modify peak width
    pv.setHeight(2. * lr_height);
    pv_fwhm = pv.fwhm();
    TS_ASSERT_DELTA(pv_fwhm, 0.5, 1.E-5);

    // make it even taller peak to change mixing
    pv.setFwhm(0.5); // accept previously set FWHM
    pv.setHeight(4. * lr_height);
    double new_mixing = pv.getParameter("Mixing");
    TS_ASSERT_DELTA(new_mixing, 1., 1.E-5);
  }

  /** Test against with pure Gaussian
   * @brief testGaussianEdge
   */
  void testGaussianEdge() {
    FunctionDomain1DVector domain(m_xValues);

    // set up and calculate Psuedo-voigt
    const double center{-1.0};
    const double intensity{2.0};
    const double fwhm{2.0};
    const double mixing{1.0};
    IPeakFunction_sptr pv = getInitializedPV(center, intensity, fwhm, mixing);
    FunctionValues valuesPV(domain);

    pv->function(domain, valuesPV);

    // check integration between numerical and analytical
    double num_intensity =
        numerical_integrate_pv(center, intensity, fwhm, mixing);
    TS_ASSERT_DELTA(num_intensity, 2.0, 1.E-5);

    // check with Gaussian at same center value, same intensity and peak width
    // This is a non-normalized Gaussian
    Gaussian gaussian;
    gaussian.initialize();
    gaussian.setCentre(-1.);
    gaussian.setIntensity(2.0);
    gaussian.setFwhm(2.);

    FunctionValues valuesGaussian(domain);
    gaussian.function(domain, valuesGaussian);

    for (size_t i = 0; i < valuesPV.size(); ++i) {
      TS_ASSERT_DELTA(valuesPV[i], valuesGaussian[i], 1e-15);
    }
  }

  void testLorentzianEdge() {
    FunctionDomain1DVector domain(m_xValues);

    const double center{-1.0};
    const double intensity{2.0};
    const double fwhm{2.0};
    const double mixing{0.};
    IPeakFunction_sptr pv = getInitializedPV(center, intensity, fwhm, mixing);
    FunctionValues valuesPV(domain);

    pv->function(domain, valuesPV);

    // check integration
    double num_intensity =
        numerical_integrate_pv(center, intensity, fwhm, mixing);
    TS_ASSERT_DELTA(num_intensity, intensity, 2.0E-2);

    Lorentzian lorentzian;
    lorentzian.initialize();
    lorentzian.setIntensity(intensity);
    lorentzian.setFwhm(fwhm);
    lorentzian.setCentre(center);
    FunctionValues valuesLorentzian(domain);
    lorentzian.function(domain, valuesLorentzian);

    TS_ASSERT_DELTA(pv->centre(), center, 1.E-10);
    TS_ASSERT_DELTA(pv->fwhm(), fwhm, 1.E-10);
    TS_ASSERT_DELTA(pv->intensity(), intensity, 1.E-10);
    TS_ASSERT_DELTA(pv->getParameter("Mixing"), mixing, 1.E-10);

    for (size_t i = 0; i < valuesPV.size(); ++i) {
      continue;
      // TS_ASSERT_DELTA(valuesPV[i], valuesLorentzian[i], 1e-15);
    }

    // check height
    TS_ASSERT_DELTA(pv->height(), lorentzian.height(), 1e-16);
  }

  /** Regular pseudo voigt
   * @brief testPseudoVoigtValues
   */
  void TONIGHT_testPseudoVoigtValues() {
    const double center{4.43};
    const double intensity{2301.0};
    const double fwhm{0.8321};
    const double mixing{0.81};
    IPeakFunction_sptr pv = getInitializedPV(center, intensity, fwhm, mixing);

    FunctionDomain1DVector domain(m_xValues);
    FunctionValues values(domain);

    pv->function(domain, values);

    for (size_t i = 0; i < values.size(); ++i) {
      std::cout << m_xValues[i] << "    " << values[i] << "\n";
    }

    // check intensity
    double numeric_intensity =
        numerical_integrate_pv(center, intensity, fwhm, mixing);
    TS_ASSERT_DELTA(numeric_intensity, intensity, 1.0E-2);
    TS_ASSERT(numeric_intensity < intensity);

    // check height
    double height = pv->height();
    TS_ASSERT_DELTA(height, values[100], 1.E-10);

    for (size_t i = 0; i < values.size(); ++i) {
      TS_ASSERT_DELTA(values[i], m_yValues[i], 1e-13);
    }
  }

  /** Test derivative with eta = 0.5
   */
  void testPseudoVoigtDerivativesXAxis() {

    IPeakFunction_sptr pv = getInitializedPV(1.0, 4.78, 0.05, 0.7);

    FunctionDomain1DVector domain(m_xValues);
    Mantid::CurveFitting::Jacobian jacobian(domain.size(), 4);

    pv->functionDeriv(domain, jacobian);

    for (size_t i = 0; i < domain.size(); ++i) {
      TS_ASSERT_DELTA(jacobian.get(i, 0), m_dfda[i], 1e-13);
      TS_ASSERT_DELTA(jacobian.get(i, 1), m_dfdh[i], 1e-13);
      TS_ASSERT_DELTA(jacobian.get(i, 2), m_dfdx0[i], 1e-11);
      TS_ASSERT_DELTA(jacobian.get(i, 3), m_dfdf[i], 1e-11);
    }
  }

  /// compare numerical derivative and analytical derivatives for eta
  void testPseudoVoigtDerivativesVaringMixing() {
    double x0 = -1.;
    double intensity = 2;
    double fwhm = 4.0;
    double min_eta = 0.4;
    double max_eta = 0.6;
    double eta_resolution = 0.005;

    // create function
    IPeakFunction_sptr pv = getInitializedPV(x0, intensity, fwhm, min_eta);
    // Mantid::CurveFitting::Jacobian jacobian(1, 4);

    // evalulate at N points
    std::vector<double> xvalues{-1}; // consider to expand to [-2, -1, 0]
    for (double x : xvalues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 4);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_eta;
      while (param_value < max_eta - eta_resolution) {
        // update eta and calcualte Jocobian
        pv->setParameter(0, param_value);
        pv->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.push_back(jacobian.get(0, 0));
        // update eta
        param_value += eta_resolution;
      }

      // calculate numerically
      std::vector<double> vec_eta;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(pv, 0, min_eta, max_eta,
                                         eta_resolution, x, vec_eta,
                                         vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_eta.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1.E-3);
      }
    }

    return;
  }

  void testPseudoVoigtDerivativesVaringIntensity() {
    double x0 = -1.;
    double min_intensity = 0.9;
    double max_intensity = 1.1;
    double fwhm = 4.0;
    double eta = 0.5;
    double intensity_resolution = 0.005;

    // create function
    IPeakFunction_sptr pv = getInitializedPV(x0, min_intensity, fwhm, eta);

    // evalulate at N points
    std::vector<double> xvalues{-1}; // consider to expand to [-2, -1, 0]
    for (double x : xvalues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 4);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_intensity;
      while (param_value < max_intensity - intensity_resolution) {
        // update intensity (index=1) and calcualte Jocobian
        pv->setParameter(1, param_value);
        pv->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.push_back(jacobian.get(0, 1));
        // update eta
        param_value += intensity_resolution;
      }

      // calculate numerically
      std::vector<double> vec_intensity;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(pv, 1, min_intensity, max_intensity,
                                         intensity_resolution, x, vec_intensity,
                                         vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_intensity.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1.E-3);
      }
    }
    return;
  }

  void testPseudoVoigtDerivativesVaringCentre() {
    double min_x0 = -1.2;
    double max_x0 = -0.8;
    double intensity = 2.;
    double fwhm = 4.0;
    double eta = 0.5;
    double x0_resolution = 0.005;

    // create function
    IPeakFunction_sptr pv = getInitializedPV(min_x0, intensity, fwhm, eta);

    // evalulate at N points
    std::vector<double> xvalues{-1}; // consider to expand to [-2, -1, 0]
    for (double x : xvalues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 4);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_x0;
      while (param_value < max_x0 - x0_resolution) {
        // update eta and calcualte Jocobian
        pv->setParameter(2, param_value);
        pv->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.push_back(jacobian.get(0, 2));
        // update eta
        param_value += x0_resolution;
      }

      // calculate numerically
      std::vector<double> vec_x0;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(pv, 2, min_x0, max_x0, x0_resolution,
                                         x, vec_x0, vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_x0.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 1.E-1);
      }
    }
    return;
  }

  void testPseudoVoigtDerivativesVaringFWHM() {
    double x0 = -1.;
    double intensity = 2;
    double min_fwhm = 3.5;
    double max_fwhm = 4.5;
    double eta = 0.5;
    double fwhm_resolution = 0.005;

    // create function
    IPeakFunction_sptr pv = getInitializedPV(x0, intensity, min_fwhm, eta);

    // evalulate at N points
    std::vector<double> xvalues{-1}; // consider to expand to [-2, -1, 0]
    for (double x : xvalues) {
      // calculate by Jacobian (analytically)
      std::vector<double> x_vec{x};
      FunctionDomain1DVector domain(x_vec);
      Mantid::CurveFitting::Jacobian jacobian(domain.size(), 4);

      std::vector<double> vec_jocob_deriv;
      double param_value = min_fwhm;
      while (param_value < max_fwhm - fwhm_resolution) {
        // update eta and calcualte Jocobian
        pv->setParameter(3, param_value);
        pv->functionDeriv(domain, jacobian);
        // get value and add to the vector
        vec_jocob_deriv.push_back(jacobian.get(0, 3));
        // update eta
        param_value += fwhm_resolution;
      }

      // calculate numerically
      std::vector<double> vec_fwhm;
      std::vector<double> vec_numeric_deriv;
      numerical_param_partial_derivative(pv, 3, min_fwhm, max_fwhm,
                                         fwhm_resolution, x, vec_fwhm,
                                         vec_numeric_deriv);

      // compare
      for (size_t i = 0; i < vec_fwhm.size(); ++i) {
        TS_ASSERT_DELTA(vec_jocob_deriv[i], vec_numeric_deriv[i], 0.005);
      }
    }
    return;
  }

private:
  IPeakFunction_sptr getInitializedPV(double center, double intensity,
                                      double fwhm, double mixing) {
    IPeakFunction_sptr pv = boost::make_shared<PseudoVoigt>();
    pv->initialize();
    pv->setParameter("PeakCentre", center);
    pv->setParameter("FWHM", fwhm);
    pv->setParameter("Mixing", mixing);
    pv->setParameter("Intensity", intensity);

    return pv;
  }

  /**
   * @brief numerical_integrate_pv : integrate PseudoVoigt numerically
   * @param center: peak center
   * @param peak_intensity: intensity
   * @param fwhm: gamma/peak width
   * @param mixing: eta/mixing ratio of Gaussian
   * @return
   */
  double numerical_integrate_pv(double center, double peak_intensity,
                                double fwhm, double mixing) {
    PseudoVoigt pv;
    pv.initialize();
    pv.setParameter(0, mixing);
    pv.setParameter(1, peak_intensity);
    pv.setParameter(2, center);
    pv.setParameter(3, fwhm);

    PeakFunctionIntegrator integrator;
    IntegrationResult result = integrator.integrate(pv, -100., 100.);

    return result.result;
  }

  /** evalulate the derivative numerically for an arbitrary parameter at x
   * calculate \partial pV() / \partial p_i  where i = 0, 1, 2 or 3 for mixing,
   * @param pv : IPeakFunction pointer to PseudoVoigt
   * @param param_index: index of the parameter (mixing: 0; I: 1; x0: 2; FHWM:
   * 3)
   * @param min_value: minimum value of the parameter to evalulate the
   * derivative
   * @param max_value: maximum value of the parameter to evalulate the
   * derivative
   * @param resolution: resolution (min step)
   * @param x:x value
   * @param param_vec: (output) parameter value vector
   * @param deriv_vec: (output) derivative vector
   */
  void numerical_param_partial_derivative(IPeakFunction_sptr &pv,
                                          size_t param_index, double min_value,
                                          double max_value, double resolution,
                                          double x,
                                          std::vector<double> &param_vec,
                                          std::vector<double> &deriv_vec) {
    // create a single value vector for domain
    std::vector<double> vec_x{x};
    FunctionDomain1DVector domain(vec_x);
    FunctionValues values(domain);

    // calculate pv value of a given X with changing parameter value
    double param_value = min_value - resolution;
    param_vec.clear();
    std::vector<double> pv_vec;
    while (param_value < max_value - resolution) {
      // update parameter and calculate
      pv->setParameter(param_index, param_value);
      pv->function(domain, values);
      // set to vector
      param_vec.push_back(param_value);
      pv_vec.push_back(values[0]);
      // increment
      param_value += resolution;
    }

    // evalulate derivative to parameter (single way dx = (f(x+h) - f(x))/h
    deriv_vec.resize(param_vec.size() - 1);
    for (size_t i = 0; i < param_vec.size() - 1; ++i) {
      deriv_vec[i] = (pv_vec[i + 1] - pv_vec[i]) / resolution;
    }
    // pop out the last element of parameter vector
    param_vec.pop_back();

    return;
  }

  std::vector<double> m_xValues;
  std::vector<double> m_yValues;
  std::vector<double> m_dfdh;
  std::vector<double> m_dfda;
  std::vector<double> m_dfdx0;
  std::vector<double> m_dfdf;
};

#endif /* MANTID_CURVEFITTING_PSEUDOVOIGTTEST_H_ */
