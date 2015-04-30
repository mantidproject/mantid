#ifndef PEAKFUNCTIONINTEGRATORTEST_H
#define PEAKFUNCTIONINTEGRATORTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/PeakFunctionIntegrator.h"

#include "gsl/gsl_errno.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class PeakFunctionIntegratorTest;

class TestablePeakFunctionIntegrator : public PeakFunctionIntegrator
{
public:
    TestablePeakFunctionIntegrator(double requiredRelativePrecision = 1e-8) :
        PeakFunctionIntegrator(requiredRelativePrecision)
    {

    }

    friend class PeakFunctionIntegratorTest;
};

class LocalGaussian : public IPeakFunction
{
public:
    LocalGaussian() : IPeakFunction() {}

    std::string name() const { return "LocalGaussian"; }

    double centre() const { return getParameter("Center"); }
    void setCentre(const double c) { setParameter("Center", c); }

    double fwhm() const { return getParameter("Sigma") * (2.0 * sqrt(2.0 * log(2.0))); }
    void setFwhm(const double w) { setParameter("Sigma", w / (2.0 * sqrt(2.0 * log(2.0)))); }

    double height() const { return getParameter("Height"); }
    void setHeight(const double h) { setParameter("Height", h); }

    void init() {
        declareParameter("Center");
        declareParameter("Sigma");
        declareParameter("Height");
    }

    void functionLocal(double *out, const double *xValues, const size_t nData) const {
        double h = getParameter("Height");
        double s = getParameter("Sigma");
        double c = getParameter("Center");

        for(size_t i = 0; i < nData; ++i) {
            out[i] = h * exp(-0.5 * pow(((xValues[i] - c)/s), 2));
        }
    }

    void functionDerivLocal(Jacobian *out, const double *xValues, const size_t nData) {
        UNUSED_ARG(out);
        UNUSED_ARG(xValues);
        UNUSED_ARG(nData);

        // Do nothing - not required for this test.
    }
};

class PeakFunctionIntegratorTest : public CxxTest::TestSuite
{
private:
    IPeakFunction_sptr getGaussian(double center, double fwhm, double height)
    {
        IPeakFunction_sptr gaussian = boost::make_shared<LocalGaussian>();
        gaussian->initialize();

        gaussian->setCentre(center);
        gaussian->setFwhm(fwhm);
        gaussian->setHeight(height);

        return gaussian;
    }

    double getGaussianAnalyticalInfiniteIntegral(IPeakFunction_sptr gaussian)
    {
        return gaussian->height() * gaussian->fwhm() / (2.0 * sqrt(2.0 * log(2.0))) * sqrt(2.0 * M_PI);
    }

public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PeakFunctionIntegratorTest *createSuite() { return new PeakFunctionIntegratorTest(); }
    static void destroySuite( PeakFunctionIntegratorTest *suite ) { delete suite; }

    void testDefaultConstruction()
    {
        TestablePeakFunctionIntegrator integrator;

        TS_ASSERT(integrator.m_integrationWorkspace != 0);
        TS_ASSERT_EQUALS(integrator.m_relativePrecision, 1e-8);
    }

    void testConstruction()
    {
        TestablePeakFunctionIntegrator integrator(1e-10);

        TS_ASSERT(integrator.m_integrationWorkspace != 0);
        TS_ASSERT_EQUALS(integrator.m_relativePrecision, 1e-10);
    }

    void testSetRequiredRelativePrecision()
    {
        PeakFunctionIntegrator integrator;
        integrator.setRequiredRelativePrecision(1e-2);

        TS_ASSERT_EQUALS(integrator.requiredRelativePrecision(), 1e-2);
    }

    void testgsl_peak_wrapper()
    {
        IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 2.0);

        TS_ASSERT_EQUALS(gsl_peak_wrapper(0.0, &(*gaussian)), 2.0);
    }

    void testIntegrateInfinityGaussian()
    {
        IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 1.0);

        PeakFunctionIntegrator integrator;
        IntegrationResult result = integrator.integrateInfinity(*gaussian);
        TS_ASSERT_EQUALS(result.errorCode, static_cast<int>(GSL_SUCCESS));
        TS_ASSERT_DELTA(result.result, getGaussianAnalyticalInfiniteIntegral(gaussian), integrator.requiredRelativePrecision());
        TS_ASSERT_DELTA(result.error, 0.0, integrator.requiredRelativePrecision());

        integrator.setRequiredRelativePrecision(1e-14);
        IntegrationResult otherResult = integrator.integrateInfinity(*gaussian);
        TS_ASSERT_EQUALS(otherResult.errorCode, static_cast<int>(GSL_EBADTOL));
        TS_ASSERT_EQUALS(otherResult.result, 0.0);
        TS_ASSERT_EQUALS(otherResult.error, 0.0);
    }

    void testIntegratePositiveInfinityGaussian()
    {
        IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 1.0);
        PeakFunctionIntegrator integrator;
        IntegrationResult result = integrator.integratePositiveInfinity(*gaussian, 0.0);

        TS_ASSERT_EQUALS(result.errorCode, static_cast<int>(GSL_SUCCESS));
        TS_ASSERT_DELTA(result.result, getGaussianAnalyticalInfiniteIntegral(gaussian) / 2.0, integrator.requiredRelativePrecision());
    }

    void testIntegrateNegativeInfinityGaussian()
    {
        IPeakFunction_sptr gaussian = getGaussian(0.0, 1.0, 1.0);
        PeakFunctionIntegrator integrator;
        IntegrationResult result = integrator.integrateNegativeInfinity(*gaussian, 0.0);

        TS_ASSERT_EQUALS(result.errorCode, static_cast<int>(GSL_SUCCESS));
        TS_ASSERT_DELTA(result.result, getGaussianAnalyticalInfiniteIntegral(gaussian) / 2.0, integrator.requiredRelativePrecision());
    }

    void testIntegrateGaussian()
    {
        /*
         * Normal distribution with mu = 0, sigma = 1, height = 1/sqrt(2 * pi)
         *  -integral from -1 to 1 should give approx. 0.682
         *  -integral from -2 to 2 should give approx. 0.954
         *  -integral from -3 to 3 should give approx. 0.997
         */
        IPeakFunction_sptr gaussian = getGaussian(0.0, 2.0 * sqrt(2.0 * log(2.0)), 1.0 / sqrt(2.0 * M_PI));
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

};

#endif // PEAKFUNCTIONINTEGRATORTEST_H
