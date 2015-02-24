#ifndef MANTID_CURVEFITTING_PEAKPARAMETERFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_PEAKPARAMETERFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PeakParameterFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/IPeakFunction.h"

#include "MantidCurveFitting/Jacobian.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class PeakParameterFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  PeakParameterFunctionTest() { FrameworkManager::Instance(); }

  static PeakParameterFunctionTest *createSuite() {
    return new PeakParameterFunctionTest();
  }
  static void destroySuite(PeakParameterFunctionTest *suite) { delete suite; }

  void testFunction() {
    FunctionParameterDecorator_sptr fn =
        boost::dynamic_pointer_cast<FunctionParameterDecorator>(
            FunctionFactory::Instance().createFunction(
                "PeakParameterFunction"));

    TS_ASSERT(fn);

    fn->setDecoratedFunction("Gaussian");

    IPeakFunction_sptr peakFunction =
        boost::dynamic_pointer_cast<IPeakFunction>(fn->getDecoratedFunction());

    FunctionDomain1DVector domain(std::vector<double>(4, 0.0));
    FunctionValues values(domain);

    fn->function(domain, values);

    TS_ASSERT_EQUALS(values[0], peakFunction->centre());
    TS_ASSERT_EQUALS(values[1], peakFunction->height());
    TS_ASSERT_EQUALS(values[2], peakFunction->fwhm());
    TS_ASSERT_EQUALS(values[3], peakFunction->intensity());
  }

  void testFunctionDeriv() {
      FunctionParameterDecorator_sptr fn =
          boost::dynamic_pointer_cast<FunctionParameterDecorator>(
              FunctionFactory::Instance().createFunction(
                  "PeakParameterFunction"));

      TS_ASSERT(fn);

      fn->setDecoratedFunction("Gaussian");

      FunctionDomain1DVector domain(std::vector<double>(4, 0.0));
      Mantid::CurveFitting::Jacobian jacobian(4, 3);

      fn->functionDeriv(domain, jacobian);

      /* Make sure that (0,1) is larger than (0,0) and (0,2)
       * because d(centre)/d(PeakCentre) should be highest.
       */
      TS_ASSERT_LESS_THAN(jacobian.get(0, 0), jacobian.get(0, 1));
      TS_ASSERT_LESS_THAN(jacobian.get(0, 2), jacobian.get(0, 1));

      // Same for d(height)/d(Height)
      TS_ASSERT_LESS_THAN(jacobian.get(1, 1), jacobian.get(1, 0));
      TS_ASSERT_LESS_THAN(jacobian.get(1, 2), jacobian.get(1, 0));

      // Same for d(fwhm)/d(Sigma)
      TS_ASSERT_LESS_THAN(jacobian.get(2, 0), jacobian.get(2, 2));
      TS_ASSERT_LESS_THAN(jacobian.get(2, 1), jacobian.get(2, 2));
  }
};

#endif /* MANTID_CURVEFITTING_PEAKPARAMETERFUNCTIONTEST_H_ */
