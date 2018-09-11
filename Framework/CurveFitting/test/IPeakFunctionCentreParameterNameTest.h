#ifndef IPEAKFUNCTIONCENTREPARAMETERNAMETEST_H
#define IPEAKFUNCTIONCENTREPARAMETERNAMETEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include <cxxtest/TestSuite.h>
#include <map>

using namespace Mantid::API;

class IPeakFunctionCentreParameterNameTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IPeakFunctionCentreParameterNameTest *createSuite() {
    return new IPeakFunctionCentreParameterNameTest();
  }
  static void destroySuite(IPeakFunctionCentreParameterNameTest *suite) {
    delete suite;
  }

  IPeakFunctionCentreParameterNameTest() {
    FrameworkManager::Instance();

    m_expectedResults.emplace("Gaussian", "PeakCentre");
    m_expectedResults.emplace("Lorentzian", "PeakCentre");
    m_expectedResults.emplace("IkedaCarpenterPV", "X0");
    m_expectedResults.emplace("Voigt", "LorentzPos");
    m_expectedResults.emplace("BackToBackExponential", "X0");
  }

  /* Test that all functions give the expected result.
   */
  void testAllFunctions() {
    for (auto &expectedResult : m_expectedResults) {
      const std::string &peakFunctionName = expectedResult.first;
      const std::string &centreParameterName = expectedResult.second;

      IPeakFunction_sptr fn = boost::dynamic_pointer_cast<IPeakFunction>(
          FunctionFactory::Instance().createFunction(peakFunctionName));

      TS_ASSERT(fn);
      TSM_ASSERT_EQUALS("IPeakFunction " + peakFunctionName +
                            " gave centre"
                            "parameter '" +
                            fn->getCentreParameterName() +
                            "', "
                            "should give '" +
                            centreParameterName + "'.",
                        fn->getCentreParameterName(), centreParameterName);
    }
  }

private:
  std::map<std::string, std::string> m_expectedResults;
};

#endif // IPEAKFUNCTIONCENTREPARAMETERNAMETEST_H
