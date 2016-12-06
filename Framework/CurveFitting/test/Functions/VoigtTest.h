#ifndef MANTID_CURVEFITTING_VOIGTTEST_H_
#define MANTID_CURVEFITTING_VOIGTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Voigt.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Jacobian.h"

#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>

using Mantid::CurveFitting::Functions::Voigt;
using Mantid::API::IFunction;
using Mantid::API::IPeakFunction;

class VoigtTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VoigtTest *createSuite() { return new VoigtTest(); }
  static void destroySuite(VoigtTest *suite) { delete suite; }

  VoigtTest() {
    using namespace Mantid::API;
    m_domain = boost::shared_ptr<FunctionDomain1DVector>(
        new FunctionDomain1DVector(-5.5, 3.5, g_domainSize));
  }

  void test_intialized_function_has_4_parameters_with_expected_names() {
    boost::scoped_ptr<IFunction> voigtFn(new Voigt);
    voigtFn->initialize();

    size_t expectedNParams(4);
    TS_ASSERT_EQUALS(expectedNParams, voigtFn->nParams());
    if (voigtFn->nParams() == 4) {
      // It is important we know if names change as they will be used in scripts
      auto names = voigtFn->getParameterNames();
      TS_ASSERT_EQUALS("LorentzAmp", names[0]);
      TS_ASSERT_EQUALS("LorentzPos", names[1]);
      TS_ASSERT_EQUALS("LorentzFWHM", names[2]);
      TS_ASSERT_EQUALS("GaussianFWHM", names[3]);
    }
  }

  void test_function_has_expected_output_given_set_input() {
    const double a_L(5), pos(-1), gamma_L(0.9), gamma_G(0.1);
    auto voigtFn = createFunction(a_L, pos, gamma_L, gamma_G);

    Mantid::API::FunctionValues outputs(*m_domain);
    voigtFn->function(*m_domain, outputs);

    double expectedOutput[g_domainSize] = {
        0.0495194770, 0.0813462678, 0.1570475305, 0.4136676242, 2.2481604925,
        2.2481604925, 0.4136676242, 0.1570475305, 0.0813462678, 0.0495194770};

    const size_t nValues(g_domainSize);
    for (size_t i = 0; i < nValues; ++i) {
      TS_ASSERT_DELTA(expectedOutput[i], outputs[i], 1e-10);
    }
  }

  void test_function_has_jacobian_matrix_for_given_input() {

    double dxDa[g_domainSize][4] = {
        {0.00990389541, -0.02179640604, 0.10895223988, 0.00026811273},
        {0.01626925356, -0.04574679593, 0.17782530103, 0.00071014811},
        {0.03140950610, -0.12178875967, 0.33801701863, 0.00256582413},
        {0.08273352484, -0.50687692061, 0.84298568363, 0.01628265653},
        {0.44963209851, -4.95613687209, 2.73013742868, 0.20944750234},
        {0.44963209851, 4.95613687209, 2.73013742868, 0.20944750234},
        {0.08273352484, 0.50687692061, 0.84298568363, 0.01628265653},
        {0.03140950610, 0.12178875967, 0.33801701863, 0.00256582413},
        {0.01626925356, 0.04574679593, 0.17782530103, 0.00071014811},
        {0.00990389541, 0.02179640604, 0.10895223988, 0.00026811273}};

    const double a_L(5), pos(-1), gamma_L(0.9), gamma_G(0.1);
    auto voigtFn = createFunction(a_L, pos, gamma_L, gamma_G);

    Mantid::CurveFitting::Jacobian jacobian(g_domainSize, 4);
    voigtFn->functionDeriv(*m_domain, jacobian);
    const size_t nValues(g_domainSize);
    for (size_t i = 0; i < nValues; ++i) {
      for (size_t j = 0; j < 4; ++j) {
        TS_ASSERT_DELTA(dxDa[i][j], jacobian.get(i, j), 1e-10);
      }
    }
  }

  void test_function_is_a_peak_function() {
    const double a_L(5), pos(-1), gamma_L(0.9), gamma_G(0.1);
    auto voigtFn = createFunction(a_L, pos, gamma_L, gamma_G);

    auto peakFn =
        boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(voigtFn);
    TSM_ASSERT("Voigt function should be a PeakFunction", peakFn);
  }

  void test_peak_functions_return_expected_results() {
    const double a_L(5), pos(-1), gamma_L(0.9), gamma_G(0.1);
    auto voigtFn = createFunction(a_L, pos, gamma_L, gamma_G);
    auto peakFn =
        boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(voigtFn);

    TS_ASSERT_DELTA(peakFn->centre(), pos, 1e-12);
    TS_ASSERT_DELTA(peakFn->height(), 4.9570, 1e-4);
    TS_ASSERT_DELTA(peakFn->fwhm(), (gamma_L + gamma_G), 1e-12);
  }

  void test_setting_peak_functions_set_expected_parameters() {
    double a_L(5), pos(-1), gamma_L(0.9), gamma_G(0.1);
    auto voigtFn = createFunction(a_L, pos, gamma_L, gamma_G);
    auto peakFn =
        boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(voigtFn);

    pos = 1.2;
    peakFn->setCentre(pos);
    TS_ASSERT_DELTA(peakFn->centre(), pos, 1e-12);

    const double height = 3.5;
    peakFn->setHeight(height);
    TS_ASSERT_DELTA(peakFn->height(), height, 1e-12);

    gamma_L = 1.2;
    gamma_G = 0.4;
    peakFn->setFwhm(gamma_L + gamma_G);
    TS_ASSERT_DELTA(peakFn->fwhm(), (gamma_L + gamma_G), 1e-12);
  }

  void test_height() {
    {
      auto voigt = createFunction(0, 0, 0, 0);
      TS_ASSERT_EQUALS(voigt->height(), 0.0);
      voigt->setHeight(3.0);
      TS_ASSERT_DELTA(voigt->height(), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 0, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 0, 1e-10);
    }
    {
      auto voigt = createFunction(1, 0, 0, 0);
      TS_ASSERT_EQUALS(voigt->height(), 0.0);
      voigt->setHeight(3.0);
      TS_ASSERT_DELTA(voigt->height(), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 0, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 0, 1e-10);
    }
    {
      auto voigt = createFunction(1, 0, 1, 0);
      TS_ASSERT_EQUALS(voigt->height(), 0.0);
      voigt->setHeight(3.0);
      TS_ASSERT_DELTA(voigt->height(), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 1, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 4.7123, 1e-4);
    }
    {
      auto voigt = createFunction(1, 0, 0, 1);
      TS_ASSERT_EQUALS(voigt->height(), 0.0);
      voigt->setHeight(3.0);
      TS_ASSERT_DELTA(voigt->height(), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 1, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 3.1933, 1e-4);
    }
    {
      auto voigt = createFunction(0, 0, 1, 1);
      TS_ASSERT_EQUALS(voigt->height(), 0.0);
      voigt->setHeight(3.0);
      TS_ASSERT_DELTA(voigt->height(), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 2, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 6.6795, 1e-4);
    }
    {
      auto voigt = createFunction(4, 0, 2, 3);
      TS_ASSERT_DELTA(voigt->height(), 2.3159, 1e-4);
      voigt->setHeight(3.0);
      TS_ASSERT_DELTA(voigt->height(), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 5, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 16.2778, 1e-4);
    }
  }

  void test_intensity() {
    {
      auto voigt = createFunction(0, 0, 0, 0);
      TS_ASSERT_EQUALS(voigt->intensity(), 0.0);
      voigt->setIntensity(3.0);
      TS_ASSERT_DELTA(voigt->height(), 6068115080134125.22, 1e10);
      TS_ASSERT_DELTA(voigt->fwhm(), 0, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 3.0, 1e-10);
    }
    {
      auto voigt = createFunction(1, 0, 0, 0);
      TS_ASSERT_EQUALS(voigt->intensity(), 0.0);
      voigt->setIntensity(3.0);
      TS_ASSERT_DELTA(voigt->height(), 6068115080134125.22, 1e10);
      TS_ASSERT_DELTA(voigt->fwhm(), 0, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 3.0, 1e-10);
    }
    {
      auto voigt = createFunction(1, 0, 0, 1);
      TS_ASSERT_EQUALS(voigt->intensity(), 0.0);
      voigt->setIntensity(3.0);
      TS_ASSERT_DELTA(voigt->height(), 2.8183, 1e-4);
      TS_ASSERT_DELTA(voigt->fwhm(), 1, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 3.0, 1e-10);
    }
    {
      auto voigt = createFunction(1, 0, 1, 0);
      TS_ASSERT_EQUALS(voigt->intensity(), 0.0);
      voigt->setIntensity(3.0);
      TS_ASSERT_DELTA(voigt->height(), 1.9098, 1e-4);
      TS_ASSERT_DELTA(voigt->fwhm(), 1, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 3.0, 1e-10);
    }
    {
      auto voigt = createFunction(4, 0, 2, 3);
      TS_ASSERT_DELTA(voigt->intensity(), 12.5663, 1e-4);
      voigt->setIntensity(3.0);
      TS_ASSERT_DELTA(voigt->height(), 0.5528, 1e-4);
      TS_ASSERT_DELTA(voigt->fwhm(), 5, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->IPeakFunction::intensity(), 3.0, 1e-2);
    }
  }

  void test_fwhm() {
    {
      auto voigt = createFunction(0, 0, 0, 0);
      TS_ASSERT_EQUALS(voigt->fwhm(), 0.0);
      voigt->setFwhm(3.0);
      TS_ASSERT_DELTA(voigt->height(), 0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 3, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 0.0, 1e-10);
      TS_ASSERT_DELTA(voigt->getParameter("LorentzFWHM"), 1.5, 1e-10);
      TS_ASSERT_DELTA(voigt->getParameter("GaussianFWHM"), 1.5, 1e-10);
    }
    {
      auto voigt = createFunction(0, 0, 1, 0);
      TS_ASSERT_EQUALS(voigt->fwhm(), 1.0);
      voigt->setFwhm(3.0);
      TS_ASSERT_DELTA(voigt->height(), 0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 3, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 0.0, 1e-10);
      TS_ASSERT_DELTA(voigt->getParameter("LorentzFWHM"), 3.0, 1e-10);
      TS_ASSERT_DELTA(voigt->getParameter("GaussianFWHM"), 0.0, 1e-10);
    }
    {
      auto voigt = createFunction(0, 0, 0, 1);
      TS_ASSERT_EQUALS(voigt->fwhm(), 1.0);
      voigt->setFwhm(3.0);
      TS_ASSERT_DELTA(voigt->height(), 0, 1e-10);
      TS_ASSERT_DELTA(voigt->fwhm(), 3, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 0.0, 1e-10);
      TS_ASSERT_DELTA(voigt->getParameter("LorentzFWHM"), 0.0, 1e-10);
      TS_ASSERT_DELTA(voigt->getParameter("GaussianFWHM"), 3.0, 1e-10);
    }
    {
      auto voigt = createFunction(2, 0, 2, 1);
      TS_ASSERT_EQUALS(voigt->fwhm(), 3.0);
      voigt->setFwhm(5.5);
      TS_ASSERT_DELTA(voigt->fwhm(), 5.5, 1e-10);
      TS_ASSERT_DELTA(voigt->intensity(), 11.5191, 1e-4);
      TS_ASSERT_DELTA(voigt->getParameter("LorentzFWHM"), 3.6666, 1e-4);
      TS_ASSERT_DELTA(voigt->getParameter("GaussianFWHM"), 1.8333, 1e-4);
    }
  }

private:
  boost::shared_ptr<Mantid::API::IPeakFunction>
  createFunction(const double a_L, const double pos, const double gamma_L,
                 const double gamma_G) {
    boost::shared_ptr<IFunction> voigtFn = boost::make_shared<Voigt>();
    auto peakFn =
        boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(voigtFn);
    voigtFn->initialize();

    voigtFn->setParameter("LorentzAmp", a_L);
    voigtFn->setParameter("LorentzPos", pos);
    voigtFn->setParameter("LorentzFWHM", gamma_L);
    voigtFn->setParameter("GaussianFWHM", gamma_G);

    return peakFn;
  }

  enum { g_domainSize = 10 };
  /// Input domain
  boost::shared_ptr<Mantid::API::FunctionDomain1DVector> m_domain;
};

#endif /* MANTID_CURVEFITTING_VOIGTTEST_H_ */
