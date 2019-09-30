// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GETEIV1TEST_H_
#define GETEIV1TEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;
using Mantid::MantidVecPtr;

class GetEiV1Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetEiV1Test *createSuite() { return new GetEiV1Test(); }
  static void destroySuite(GetEiV1Test *suite) { delete suite; }

  GetEiV1Test() {
    FrameworkManager::Instance(); // Load plugins
  }

  void test_Result_For_Good_Estimate() {
    const double input_ei = 15.0;
    const bool fixei = false;
    do_test_on_result_values(input_ei, fixei);
  }

  void do_test_on_result_values(double input_ei, bool fixei) {
    MatrixWorkspace_sptr testWS = createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(
        alg = runGetEiUsingTestMonitors(outputName, input_ei));

    // Test output answers
    // The monitor peak should always be calculated from the data
    const double expected_mon_peak = 6495.7499801169;
    const double expected_ei = (fixei) ? input_ei : 15.001453367;
    const double ei = alg->getProperty("IncidentEnergy");
    const double first_mon_peak = alg->getProperty("FirstMonitorPeak");

    TS_ASSERT_DELTA(ei, expected_ei, 1e-08);
    TS_ASSERT_DELTA(first_mon_peak, expected_mon_peak, 1e-08);

    // and verify it has been store on the run object
    Property *ei_runprop = testWS->run().getProperty("Ei");
    PropertyWithValue<double> *ei_propvalue =
        dynamic_cast<PropertyWithValue<double> *>(ei_runprop);
    TS_ASSERT_DELTA((*ei_propvalue)(), expected_ei, 1e-08);

    AnalysisDataService::Instance().remove(outputName);
  }

  void testParametersOnWorkspace() {
    MatrixWorkspace_sptr testWS = createTestWorkspaceWithMonitors();

    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getChild(0).get(), "ei-mon1-spec", "1");
    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getChild(0).get(), "ei-mon2-spec", "2");
    Property *incident_energy_guess =
        new PropertyWithValue<double>("EnergyRequest", 15.0, Direction::Input);
    testWS->mutableRun().addProperty(incident_energy_guess, true);
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eiNoParTest");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = runGetEiUsingTestMonitors(outputName, 15));

    // Test output answers
    const double expected_ei = 15.001453367;
    const double ei = alg->getProperty("IncidentEnergy");

    TS_ASSERT_DELTA(ei, expected_ei, 1e-08);
    // and verify it has been store on the run object
    Property *ei_runprop = testWS->run().getProperty("Ei");
    PropertyWithValue<double> *ei_propvalue =
        dynamic_cast<PropertyWithValue<double> *>(ei_runprop);
    TS_ASSERT_DELTA((*ei_propvalue)(), expected_ei, 1e-08);

    AnalysisDataService::Instance().remove(outputName);
  }

  void testThrowsMon1() {
    MatrixWorkspace_sptr testWS = createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest1");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("GetEi", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outputName);
    alg->setProperty("Monitor2Spec", 2);
    alg->setProperty("EnergyEstimate", 15.0);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e,
                            std::string(e.what()),
                            "Some invalid Properties found");
    AnalysisDataService::Instance().remove(outputName);
  }
  void testThrowsEi() {
    MatrixWorkspace_sptr testWS = createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest2");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("GetEi", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outputName);
    alg->setProperty("Monitor1Spec", 1);
    alg->setProperty("Monitor2Spec", 2);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e,
                            std::string(e.what()),
                            "Some invalid Properties found");
    AnalysisDataService::Instance().remove(outputName);
  }

  void test_throws_error_when_ei_not_fixed_and_no_peaks_found() {
    const bool includePeaks(false);
    MatrixWorkspace_sptr testWS = createTestWorkspaceWithMonitors(includePeaks);
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest2");
    AnalysisDataService::Instance().add(outputName, testWS);

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("GetEi", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outputName);
    alg->setProperty("Monitor1Spec", 1);
    alg->setProperty("Monitor2Spec", 2);
    alg->setProperty("EnergyEstimate", 15.0);
    alg->setRethrows(true);
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);

    AnalysisDataService::Instance().remove(outputName);
  }

private:
  MatrixWorkspace_sptr
  createTestWorkspaceWithMonitors(const bool includePeaks = true) {
    const int numHists(2);
    const int numBins(2000);
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            numHists, numBins, true);
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
    BinEdges xdata(numBins + 1, LinearGenerator(5.0, 5.5));
    // Update X data  to a sensible values. Looks roughly like the MARI binning
    // Update the Y values. We don't care about errors here

    // Instrument geometry + incident energy of ~15 mev (purely made up) gives
    // these neceesary peak values.
    // We'll simply use a gaussian as a test

    const double peakOneCentre(6493.0), sigmaSqOne(150. * 150.),
        peakTwoCentre(10625.), sigmaSqTwo(25. * 25.);
    const double peakOneHeight(3000.), peakTwoHeight(1000.);
    for (int i = 0; i < numBins; ++i) {
      if (includePeaks) {
        testWS->dataY(0)[i] =
            peakOneHeight *
            exp(-0.5 * pow(xdata[i] - peakOneCentre, 2.) / sigmaSqOne);
        testWS->dataY(1)[i] =
            peakTwoHeight *
            exp(-0.5 * pow(xdata[i] - peakTwoCentre, 2.) / sigmaSqTwo);
      }
    }
    testWS->setBinEdges(0, xdata);
    testWS->setBinEdges(1, xdata);
    return testWS;
  }

  IAlgorithm_sptr runGetEiUsingTestMonitors(const std::string &inputWS,
                                            const double energyGuess) {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("GetEi", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWS);
    alg->setProperty("Monitor1Spec", 1);
    alg->setProperty("Monitor2Spec", 2);
    alg->setProperty("EnergyEstimate", energyGuess);
    alg->setRethrows(true);
    alg->execute();
    return alg;
  }
};

#endif /*GETEITEST_H_*/
