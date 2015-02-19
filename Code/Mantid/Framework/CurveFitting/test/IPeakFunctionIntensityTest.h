#ifndef IPEAKFUNCTIONINTENSITYTEST_H
#define IPEAKFUNCTIONINTENSITYTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;

#define DBL2STR(x) boost::lexical_cast<std::string>(x)

struct ParameterSet {
  ParameterSet(double c, double h, double f) : center(c), height(h), fwhm(f) {}

  double center;
  double height;
  double fwhm;
};

class IPeakFunctionIntensityTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IPeakFunctionIntensityTest *createSuite() {
    return new IPeakFunctionIntensityTest();
  }
  static void destroySuite(IPeakFunctionIntensityTest *suite) { delete suite; }

  IPeakFunctionIntensityTest() : m_blackList() {
    FrameworkManager::Instance();

    m_blackList.insert("BackToBackExponential");
    m_blackList.insert("DeltaFunction");
    m_blackList.insert("ElasticDiffRotDiscreteCircle");
    m_blackList.insert("ElasticDiffSphere");

    m_peakFunctions = getAllPeakFunctions(m_blackList);
    m_parameterSets = getParameterSets();
  }

  /* This test sets all peak function parameters (center, fwhm, height)
   * to the values supplied in the first ParameterSet contained in
   * m_parameterSets.
   *
   * Then it retrieves the intensities of the peak functions and stores them.
   * Each time new parameters are set, the ratio of the height parameter to
   * the previous step is compared to the intensity ratio - they should be
   * the same.
   */
  void testAllFunctions() {
    initializePeakFunctions(m_peakFunctions, m_parameterSets[0]);

    std::vector<double> initialIntensities = getIntensities(m_peakFunctions);

    for (size_t i = 1; i < m_parameterSets.size(); ++i) {
      double oldHeight = m_parameterSets[i - 1].height;
      double newHeight = m_parameterSets[i].height;
      double heightRatio = newHeight / oldHeight;

      initializePeakFunctions(m_peakFunctions, m_parameterSets[i]);

      std::vector<double> newIntensities = getIntensities(m_peakFunctions);

      for (size_t j = 0; j < initialIntensities.size(); ++j) {
        double oldIntensity = initialIntensities[j];
        double newIntensity = newIntensities[j];
        double intensityRatio = newIntensity / oldIntensity;

        TSM_ASSERT_DELTA(
            "ITERATION " + DBL2STR(i) + ", " + m_peakFunctions[j]->name() +
                ": Height was increased from " + DBL2STR(oldHeight) + " to " +
                DBL2STR(newHeight) + " (ratio " + DBL2STR(heightRatio) +
                "), but intensity changed from " + DBL2STR(oldIntensity) +
                " to " + DBL2STR(newIntensity) + " (ratio " +
                DBL2STR(intensityRatio) + ").",
            intensityRatio, heightRatio, 1e-10);
      }

      initialIntensities = newIntensities;
    }
  }

private:
  std::vector<IPeakFunction_sptr>
  getAllPeakFunctions(const std::set<std::string> &blackList) const {
    std::vector<IPeakFunction_sptr> peakFunctions;

    std::vector<std::string> registeredFunctions =
        FunctionFactory::Instance().getFunctionNames<IPeakFunction>();

    for (auto it = registeredFunctions.begin(); it != registeredFunctions.end();
         ++it) {
      if (blackList.count(*it) == 0) {
        IPeakFunction_sptr peakFunction =
            boost::dynamic_pointer_cast<IPeakFunction>(
                FunctionFactory::Instance().createFunction(*it));

        if (peakFunction) {
          peakFunctions.push_back(peakFunction);
        }
      }
    }

    return peakFunctions;
  }

  void initializePeakFunctions(const std::vector<IPeakFunction_sptr> &peaks,
                               const ParameterSet &parameters) const {

    for (auto it = peaks.begin(); it != peaks.end(); ++it) {
      (*it)->setCentre(parameters.center);

      // for Ikeda-Carpenter it's not allowed to set Fwhm
      try {
        (*it)->setFwhm(parameters.fwhm);
      }
      catch (std::invalid_argument) {
      }

      (*it)->setHeight(parameters.height);
    }
  }

  std::vector<ParameterSet> getParameterSets() const {
    std::vector<ParameterSet> parameterSets;
    parameterSets.push_back(ParameterSet(0.0, 4.34, 0.25));
    parameterSets.push_back(ParameterSet(0.0, 5.34, 0.25));
    parameterSets.push_back(ParameterSet(0.0, 6.34, 0.25));
    parameterSets.push_back(ParameterSet(0.0, 7.34, 0.25));

    return parameterSets;
  }

  std::vector<double>
  getIntensities(const std::vector<IPeakFunction_sptr> &peaks) const {
    std::vector<double> intensities;

    for (auto it = peaks.begin(); it != peaks.end(); ++it) {
      intensities.push_back((*it)->intensity());
    }

    return intensities;
  }

  std::vector<IPeakFunction_sptr> m_peakFunctions;
  std::vector<ParameterSet> m_parameterSets;
  std::set<std::string> m_blackList;
};

#endif // IPEAKFUNCTIONINTENSITYTEST_H
