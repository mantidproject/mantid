// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/PeakFunctionIntegrator.h"

#include <cxxtest/TestSuite.h>

#include "gsl/gsl_errno.h"
#include <memory>

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class PeakFunctionIntegratorTest;

class TestablePeakFunctionIntegrator : public PeakFunctionIntegrator {
public:
  TestablePeakFunctionIntegrator(double requiredRelativePrecision = 1e-8)
      : PeakFunctionIntegrator(requiredRelativePrecision) {}

  friend class PeakFunctionIntegratorTest;
};

class LocalGaussian : public IPeakFunction {
public:
  LocalGaussian() : IPeakFunction() {}

  std::string name() const override { return "LocalGaussian"; }

  double centre() const override { return getParameter("Center"); }
  void setCentre(const double c) override { setParameter("Center", c); }

  double fwhm() const override { return getParameter("Sigma") * (2.0 * sqrt(2.0 * M_LN2)); }
  void setFwhm(const double w) override { setParameter("Sigma", w / (2.0 * sqrt(2.0 * M_LN2))); }

  double height() const override { return getParameter("Height"); }
  void setHeight(const double h) override { setParameter("Height", h); }

  void setParameterErrors(double heightError, double sigmaError, double centerError) {
    setError("Height", heightError);
    setError("Sigma", sigmaError);
    setError("Center", centerError);
  }

  void init() override {
    declareParameter("Center");
    declareParameter("Sigma");
    declareParameter("Height");
  }

  void functionLocal(double *out, const double *xValues, const size_t nData) const override {
    double h = getParameter("Height");
    double s = getParameter("Sigma");
    double c = getParameter("Center");

    for (size_t i = 0; i < nData; ++i) {
      out[i] = h * exp(-0.5 * pow(((xValues[i] - c) / s), 2));
    }
  }

  /// Error in the integrated intensity due to error in the optimized fit parameters (assumed uncorrelated)
  double intensityErrorLocal() {
    const double h = getParameter("Height");
    const double hE = getError("Height");
    const double s = getParameter("Sigma");
    const double sE = getError("Sigma");
    return intensity() * sqrt(pow(hE / h, 2) + pow(sE / s, 2));
  }
};

// Declaration to the function factory is required if we want to clone LocalGaussian, as is done in
// PeakFunctionIntegrator::intensityError
DECLARE_FUNCTION(LocalGaussian)

class PeakFunctionIntegratorTest : public CxxTest::TestSuite {
private:
  IPeakFunction_sptr getGaussian(double center, double fwhm, double height) {
    IPeakFunction_sptr gaussian = std::make_shared<LocalGaussian>();
    gaussian->initialize();

    gaussian->setCentre(center);
    gaussian->setFwhm(fwhm);
    gaussian->setHeight(height);

    return gaussian;
  }

  double getGaussianAnalyticalInfiniteIntegral(const IPeakFunction_sptr &gaussian) {
    return gaussian->height() * gaussian->fwhm() / (2.0 * sqrt(M_LN2)) * sqrt(M_PI);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakFunctionIntegratorTest *createSuite() { return new PeakFunctionIntegratorTest(); }
  static void destroySuite(PeakFunctionIntegratorTest *suite) { delete suite; }

  void testDefaultConstruction() {
    TestablePeakFunctionIntegrator integrator;

    TS_ASSERT(integrator.m_integrationWorkspace != nullptr);
    TS_ASSERT_EQUALS(integrator.m_relativePrecision, 1e-8);
  }

  void testConstruction() {
    TestablePeakFunctionIntegrator integrator(1e-10);

    TS_ASSERT(integrator.m_integrationWorkspace != nullptr);
    TS_ASSERT_EQUALS(integrator.m_relativePrecision, 1e-10);
  }

  void testSetRequiredRelativePrecision() {
    PeakFunctionIntegrator integrator;
    integrator.setRequiredRelativePrecision(1e-2);

    TS_ASSERT_EQUALS(integrator.requiredRelativePrecision(), 1e-2);
  }

  void testgsl_peak_wrapper() {
    IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 2.0);

    TS_ASSERT_EQUALS(gsl_peak_wrapper(0.0, &(*gaussian)), 2.0);
  }

  void testIntegrateInfinityGaussian() {
    IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 1.0);

    TestablePeakFunctionIntegrator integrator;
    IntegrationResult result = integrator.integrateInfinity(*gaussian);
    TS_ASSERT_EQUALS(result.errorCode, static_cast<int>(GSL_SUCCESS));
    TS_ASSERT_DELTA(result.result, getGaussianAnalyticalInfiniteIntegral(gaussian),
                    integrator.requiredRelativePrecision());
    TS_ASSERT_DELTA(result.error, 0.0, integrator.requiredRelativePrecision());

    integrator.setRequiredRelativePrecision(1e-14);
    IntegrationResult otherResult = integrator.integrateInfinity(*gaussian);
    TS_ASSERT_EQUALS(otherResult.errorCode, static_cast<int>(GSL_EBADTOL));
    TS_ASSERT_EQUALS(otherResult.result, 0.0);
    TS_ASSERT_EQUALS(otherResult.error, 0.0);
  }

  void testIntegratePositiveInfinityGaussian() {
    IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 1.0);
    PeakFunctionIntegrator integrator;
    IntegrationResult result = integrator.integratePositiveInfinity(*gaussian, 0.0);

    TS_ASSERT_EQUALS(result.errorCode, static_cast<int>(GSL_SUCCESS));
    TS_ASSERT_DELTA(result.result, getGaussianAnalyticalInfiniteIntegral(gaussian) / 2.0,
                    integrator.requiredRelativePrecision());
  }

  void testIntegrateNegativeInfinityGaussian() {
    IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 1.0);
    PeakFunctionIntegrator integrator;
    IntegrationResult result = integrator.integrateNegativeInfinity(*gaussian, 0.0);

    TS_ASSERT_EQUALS(result.errorCode, static_cast<int>(GSL_SUCCESS));
    TS_ASSERT_DELTA(result.result, getGaussianAnalyticalInfiniteIntegral(gaussian) / 2.0,
                    integrator.requiredRelativePrecision());
  }

  void testIntegrateGaussian() {
    /*
     * Normal distribution with mu = 0, sigma = 1, height = 1/sqrt(2 * pi)
     *  -integral from -1 to 1 should give approx. 0.682
     *  -integral from -2 to 2 should give approx. 0.954
     *  -integral from -3 to 3 should give approx. 0.997
     */
    IPeakFunction_sptr gaussian = getGaussian(0.0, 2.0 * sqrt(2.0 * M_LN2), 1.0 / sqrt(2.0 * M_PI));
    PeakFunctionIntegrator integrator(1e-10);

    IntegrationResult rOneSigma = integrator.integrate(*gaussian, -1.0, 1.0);
    TS_ASSERT_EQUALS(rOneSigma.errorCode, static_cast<int>(GSL_SUCCESS));
    TS_ASSERT_DELTA(rOneSigma.result, 0.682689492137086, integrator.requiredRelativePrecision());

    IntegrationResult rTwoSigma = integrator.integrate(*gaussian, -2.0, 2.0);
    TS_ASSERT_EQUALS(rTwoSigma.errorCode, static_cast<int>(GSL_SUCCESS));
    TS_ASSERT_DELTA(rTwoSigma.result, 0.954499736103642, integrator.requiredRelativePrecision());

    IntegrationResult rThreeSigma = integrator.integrate(*gaussian, -3.0, 3.0);
    TS_ASSERT_EQUALS(rThreeSigma.errorCode, static_cast<int>(GSL_SUCCESS));
    TS_ASSERT_DELTA(rThreeSigma.result, 0.997300203936740, integrator.requiredRelativePrecision());
  }

  void testIntegrateErrorGaussian() {
    PeakFunctionIntegrator integrator(1e-10);
    double halfRange = 10.0; // we'll integrate in the interval [-halfRange, halfRange]

    // Normal distribution with mu = 0, sigma = 1, height = 1/sqrt(2 * pi)
    auto gaussian = LocalGaussian();
    gaussian.initialize();
    gaussian.setCentre(0.0);
    gaussian.setHeight(1.0 / sqrt(2.0 * M_PI));
    gaussian.setFwhm(2.0 * sqrt(2.0 * M_LN2));

    // No errors should return Nan
    double error = integrator.integrateError(gaussian, -halfRange, halfRange);
    TS_ASSERT(std::isnan(error));

    // Shifting the gaussian peak much less than the integration range doesn't alter the integrated intensity error
    gaussian.setParameterErrors(0.0, 0.0, 0.1);
    error = integrator.integrateError(gaussian, -halfRange, halfRange);
    TS_ASSERT_DELTA(0.0, error, 1.e-06);

    // General case of uncorrelated errors
    gaussian.setParameterErrors(0.04, 0.1, 0.0);
    error = integrator.integrateError(gaussian, -halfRange, halfRange);
    TS_ASSERT_DELTA(gaussian.intensityErrorLocal(), error, 1.e-06);
  }
};
