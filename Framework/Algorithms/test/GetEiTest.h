// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::MantidVecPtr;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;

namespace GetEiTestHelper {
IAlgorithm_sptr runGetEiUsingTestMonitors(const std::string &inputWS, const double energyGuess, const bool fixei) {
  auto alg = AlgorithmManager::Instance().createUnmanaged("GetEi", 2);
  alg->initialize();
  alg->setPropertyValue("InputWorkspace", inputWS);
  alg->setProperty("Monitor1Spec", 1);
  alg->setProperty("Monitor2Spec", 2);
  alg->setProperty("FixEi", fixei);
  alg->setProperty("EnergyEstimate", energyGuess);
  alg->setRethrows(true);
  alg->execute();
  return alg;
}

MatrixWorkspace_sptr createTestWorkspaceWithMonitors(const bool includePeaks = true) {
  const int numHists(2);
  const int numBins(2000);
  MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numHists, numBins, true);
  testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
  // Update X data  to a sensible values. Looks roughly like the MARI binning
  // Update the Y values. We don't care about errors here

  // Instrument geometry + incident energy of ~15 mev (purely made up) gives
  // these neceesary peak values.
  // We'll simply use a gaussian as a test

  const double peakOneCentre(6493.0), sigmaSqOne(250 * 250.), peakTwoCentre(10625.), sigmaSqTwo(50 * 50);
  const double peakOneHeight(3000.), peakTwoHeight(1000.);

  BinEdges xdata(numBins + 1, LinearGenerator(5.0, 5.5));

  // xdata.end() - 1 because bin edges are 1 bigger than Y's size
  if (includePeaks) {
    std::transform(xdata.cbegin(), xdata.cend() - 1, testWS->mutableY(0).begin(),
                   [peakOneHeight, peakOneCentre, sigmaSqOne](const double x) {
                     return peakOneHeight * exp(-0.5 * pow(x - peakOneCentre, 2.) / sigmaSqOne);
                   });

    std::transform(xdata.cbegin(), xdata.cend() - 1, testWS->mutableY(1).begin(),
                   [peakTwoHeight, peakTwoCentre, sigmaSqTwo](const double x) {
                     return peakTwoHeight * exp(-0.5 * pow(x - peakTwoCentre, 2.) / sigmaSqTwo);
                   });
  }

  testWS->setBinEdges(0, xdata);
  testWS->setBinEdges(1, xdata);

  return testWS;
}
} // namespace GetEiTestHelper
class GetEiTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetEiTest *createSuite() { return new GetEiTest(); }
  static void destroySuite(GetEiTest *suite) { delete suite; }

  GetEiTest() {
    FrameworkManager::Instance(); // Load plugins
  }

  void test_Result_For_Good_Estimate() {
    const double input_ei = 15.0;
    const bool fixei = false;
    do_test_on_result_values(input_ei, fixei);
  }

  void test_Result_When_Fixing_Ei() {
    const double input_ei = 15.0;
    const bool fixei = true;
    do_test_on_result_values(input_ei, fixei);
  }

  void do_test_on_result_values(double input_ei, bool fixei) {
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = GetEiTestHelper::runGetEiUsingTestMonitors(outputName, input_ei, fixei));

    // Test output answers
    // The monitor peak should always be calculated from the data
    const double expected_mon_peak = 6496.00571578;
    const int expected_mon_index = 0;
    const double expected_ei = (fixei) ? input_ei : 15.00322845;
    const double ei = alg->getProperty("IncidentEnergy");
    const double first_mon_peak = alg->getProperty("FirstMonitorPeak");
    const int mon_index = alg->getProperty("FirstMonitorIndex");

    TS_ASSERT_DELTA(ei, expected_ei, 1e-08);
    TS_ASSERT_DELTA(first_mon_peak, expected_mon_peak, 1e-08);
    TS_ASSERT_EQUALS(mon_index, expected_mon_index);
    // and verify it has been store on the run object
    Property *ei_runprop = testWS->run().getProperty("Ei");
    PropertyWithValue<double> *ei_propvalue = dynamic_cast<PropertyWithValue<double> *>(ei_runprop);
    TS_ASSERT_DELTA((*ei_propvalue)(), expected_ei, 1e-08);

    const Mantid::Kernel::Property *tzeroProp = alg->getProperty("Tzero");
    if (fixei) {
      TS_ASSERT(tzeroProp->isDefault());
    } else {
      // T0 value
      const double tzero = alg->getProperty("Tzero");
      const double expected_tzero = 3.2641273;
      TS_ASSERT_DELTA(tzero, expected_tzero, 1e-08);
    }

    AnalysisDataService::Instance().remove(outputName);
  }

  void testParametersOnWorkspace() {
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors();

    testWS->instrumentParameters().addString(testWS->getInstrument()->getChild(0).get(), "ei-mon1-spec", "1");
    testWS->instrumentParameters().addString(testWS->getInstrument()->getChild(0).get(), "ei-mon2-spec", "2");
    Property *incident_energy_guess = new PropertyWithValue<double>("EnergyRequest", 15.0, Direction::Input);
    testWS->mutableRun().addProperty(incident_energy_guess, true);
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eiNoParTest");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = GetEiTestHelper::runGetEiUsingTestMonitors(outputName, 15, false));

    // Test output answers
    const double expected_ei = 15.00322845;
    const double ei = alg->getProperty("IncidentEnergy");

    TS_ASSERT_DELTA(ei, expected_ei, 1e-08);
    // and verify it has been store on the run object
    Property *ei_runprop = testWS->run().getProperty("Ei");
    PropertyWithValue<double> *ei_propvalue = dynamic_cast<PropertyWithValue<double> *>(ei_runprop);
    TS_ASSERT_DELTA((*ei_propvalue)(), expected_ei, 1e-08);

    // T0 value
    const double tzero = alg->getProperty("Tzero");
    const double expected_tzero = 3.2641273;
    TS_ASSERT_DELTA(tzero, expected_tzero, 1e-08);

    AnalysisDataService::Instance().remove(outputName);
  }

  void testThrowsMon1() {
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest1");
    AnalysisDataService::Instance().add(outputName, testWS);

    auto alg = AlgorithmManager::Instance().createUnmanaged("GetEi", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outputName);
    alg->setProperty("Monitor2Spec", 2);
    alg->setProperty("EnergyEstimate", 15.0);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::invalid_argument &e, std::string(e.what()),
                            "Could not determine spectrum number to use. Try to set it explicitly");
    AnalysisDataService::Instance().remove(outputName);
  }
  void testThrowsEi() {
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest2");
    AnalysisDataService::Instance().add(outputName, testWS);

    auto alg = AlgorithmManager::Instance().createUnmanaged("GetEi", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outputName);
    alg->setProperty("Monitor1Spec", 1);
    alg->setProperty("Monitor2Spec", 2);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::invalid_argument &e, std::string(e.what()),
                            "Could not find an energy guess");
    AnalysisDataService::Instance().remove(outputName);
  }

  void test_throws_error_when_ei_not_fixed_and_no_peaks_found() {
    const bool includePeaks(false);
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors(includePeaks);
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest2");
    AnalysisDataService::Instance().add(outputName, testWS);

    auto alg = AlgorithmManager::Instance().createUnmanaged("GetEi", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outputName);
    alg->setProperty("Monitor1Spec", 1);
    alg->setProperty("Monitor2Spec", 2);
    alg->setProperty("FixEi", false);
    alg->setProperty("EnergyEstimate", 15.0);
    alg->setRethrows(true);
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);

    AnalysisDataService::Instance().remove(outputName);
  }

  void test_peak_time_is_guess_time_when_ei_fixed_and_no_peak_found() {
    const bool includePeaks(false);
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors(includePeaks);
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest2");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = GetEiTestHelper::runGetEiUsingTestMonitors(outputName, 15.0, true));
    if (alg) {
      const double firstMonPeak = alg->getProperty("FirstMonitorPeak");
      TS_ASSERT_DELTA(firstMonPeak, 6493.4402, 1e-4);
    }

    AnalysisDataService::Instance().remove(outputName);
  }

  void testCNCS() {
    auto ld = AlgorithmManager::Instance().createUnmanaged("LoadNexusMonitors");
    std::string outws_name = "cncs";
    ld->initialize();
    ld->setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld->setPropertyValue("OutputWorkspace", outws_name);

    ld->execute();
    TS_ASSERT(ld->isExecuted());

    auto alg = AlgorithmManager::Instance().createUnmanaged("GetEi", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outws_name);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    // T0 value
    const double tzero = alg->getProperty("Tzero");
    const double expected_tzero = 61.7708;
    TS_ASSERT_DELTA(tzero, expected_tzero, 1e-04);

    AnalysisDataService::Instance().remove(outws_name);
  }

  /**
   * Modify the test workspace so that one of the monitors has a local maximum on the trailing edge
   * of the peak. Preivously, this would result in a divide-by-zero error.
   */
  void testNoisyDataOnTrailingEdge() {
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors();

    auto &yValues = testWS->mutableY(0);
    // Find index of peak value and calcualte half height.
    int64_t iPeak = std::max_element(yValues.cbegin(), yValues.cend()) - yValues.begin();
    double yPeak = yValues[iPeak];
    double halfHeight = yPeak * 0.5;

    // Find index of the point before y goes below half-height on the trailing edge.
    int nValues = static_cast<int>(yValues.size());
    int64_t iBeforeHalfHeight = 0;
    for (int64_t i = iPeak; i < nValues; ++i) {
      if (yValues[i] < halfHeight) {
        iBeforeHalfHeight = i - 1;
        break;
      }
    }

    // To recreate the bug, need to have the value at ip == half height, a point after it
    // with a greater value and the point just after that equal to half-height.
    yValues[iBeforeHalfHeight] = halfHeight;
    yValues[iBeforeHalfHeight + 3] = halfHeight + 1;
    yValues[iBeforeHalfHeight + 4] = yValues[iBeforeHalfHeight];

    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitestNoisyData");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = GetEiTestHelper::runGetEiUsingTestMonitors(outputName, 15.0, false));

    AnalysisDataService::Instance().remove(outputName);
  }

  /**
   * Modify the test workspace so that one of the monitors has a local maximum on the leading edge
   * of the peak. Preivously, this would result in a divide-by-zero error.
   */
  void testNoisyDataOnLeadingEdge() {
    MatrixWorkspace_sptr testWS = GetEiTestHelper::createTestWorkspaceWithMonitors();

    auto &yValues = testWS->mutableY(0);
    // Find index of peak value and calcualte half height.
    int64_t iPeak = std::max_element(yValues.cbegin(), yValues.cend()) - yValues.begin();
    double yPeak = yValues[iPeak];
    double halfHeight = yPeak * 0.5;

    // Find index of the point after y goes below half-height on the leading edge.
    int64_t iAfterHalfHeight = 0;
    for (int64_t i = iPeak; i >= 0; --i) {
      if (yValues[i] < halfHeight) {
        iAfterHalfHeight = i + 1;
        break;
      }
    }

    // To recreate the bug, need to have the value at ip == half height, a point before it
    // with a greater value and the point just before that equal to half-height.
    yValues[iAfterHalfHeight] = halfHeight;
    yValues[iAfterHalfHeight - 3] = halfHeight + 1;
    yValues[iAfterHalfHeight - 4] = yValues[iAfterHalfHeight];

    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitestNoisyData");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = GetEiTestHelper::runGetEiUsingTestMonitors(outputName, 15.0, false));

    AnalysisDataService::Instance().remove(outputName);
  }
};

class GetEiTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetEiTestPerformance *createSuite() { return new GetEiTestPerformance(); }
  static void destroySuite(GetEiTestPerformance *suite) { delete suite; }

  void setUp() override {
    getEiTest = GetEiTest();
    inputWS1 = GetEiTestHelper::createTestWorkspaceWithMonitors();
    outputName1 = "eitest1";
    AnalysisDataService::Instance().add(outputName1, inputWS1);

    inputWS2 = GetEiTestHelper::createTestWorkspaceWithMonitors();
    outputName2 = "eitest2";
    AnalysisDataService::Instance().add(outputName2, inputWS1);

    // This algorithm needs a name attached to the workspace
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove(outputName1);
    AnalysisDataService::Instance().remove(outputName2);
  }

  void test_Result_For_Good_Estimate() {
    const double input_ei = 15.0;
    const bool fixei = false;

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = GetEiTestHelper::runGetEiUsingTestMonitors(outputName1, input_ei, fixei));
  }

  void test_Result_When_Fixing_Ei() {
    const double input_ei = 15.0;
    const bool fixei = true;

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = GetEiTestHelper::runGetEiUsingTestMonitors(outputName2, input_ei, fixei));
  }

  MatrixWorkspace_sptr inputWS1;
  MatrixWorkspace_sptr inputWS2;
  std::string outputName1;
  std::string outputName2;
  // The test method object so that we can re-use the methods
  GetEiTest getEiTest;
};
