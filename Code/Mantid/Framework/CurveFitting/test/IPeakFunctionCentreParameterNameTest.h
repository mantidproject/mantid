#ifndef IPEAKFUNCTIONCENTREPARAMETERNAMETEST_H
#define IPEAKFUNCTIONCENTREPARAMETERNAMETEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
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

    m_expectedResults.insert(std::make_pair("Gaussian", "PeakCentre"));
    m_expectedResults.insert(std::make_pair("Lorentzian", "PeakCentre"));
    m_expectedResults.insert(std::make_pair("IkedaCarpenterPV", "X0"));
    m_expectedResults.insert(std::make_pair("Voigt", "LorentzPos"));
    m_expectedResults.insert(std::make_pair("BackToBackExponential", "X0"));
  }

  /* Test that all functions give the expected result.
   */
  void testAllFunctions() {
    for (auto it = m_expectedResults.begin(); it != m_expectedResults.end();
         ++it) {
      std::string peakFunctionName = it->first;
      std::string centreParameterName = it->second;

      IPeakFunction_sptr fn = boost::dynamic_pointer_cast<IPeakFunction>(
          FunctionFactory::Instance().createFunction(peakFunctionName));

      TS_ASSERT(fn);
      TSM_ASSERT_EQUALS("IPeakFunction " + peakFunctionName + " gave centre"
                        "parameter '" + fn->getCentreParameterName() + "', "
                        "should give '" + centreParameterName + "'.",
                        fn->getCentreParameterName(), centreParameterName);
    }
  }

private:
  std::map<std::string, std::string> m_expectedResults;
};

#endif // IPEAKFUNCTIONCENTREPARAMETERNAMETEST_H
