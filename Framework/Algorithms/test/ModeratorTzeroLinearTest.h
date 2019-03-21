// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MODERATORTZEROLINEARTEST_H_
#define MANTID_ALGORITHMS_MODERATORTZEROLINEARTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ModeratorTzeroLinear.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;
using Mantid::Types::Event::TofEvent;

namespace {
void addToInstrument(MatrixWorkspace_sptr testWS,
                     const bool &add_deltaE_mode = false,
                     const bool &add_t0_formula = false) {
  const double evalue(2.082); // energy corresponding to the first order Bragg
                              // peak in the analyzers
  if (add_deltaE_mode) {
    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getComponentID(), "deltaE-mode", "indirect");
    auto &pmap = testWS->instrumentParameters();
    const auto &spectrumInfo = testWS->spectrumInfo();
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      pmap.addDouble(&spectrumInfo.detector(ihist), "Efixed", evalue);
    }
  }
  if (add_t0_formula) {
    testWS->instrumentParameters().addDouble(
        testWS->getInstrument()->getComponentID(),
        "Moderator.TimeZero.gradient", 11.0);
    testWS->instrumentParameters().addDouble(
        testWS->getInstrument()->getComponentID(),
        "Moderator.TimeZero.intercept", -5.0);
  }
}
} // namespace

class ModeratorTzeroLinearTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModeratorTzeroLinearTest *createSuite() {
    return new ModeratorTzeroLinearTest();
  }
  static void destroySuite(ModeratorTzeroLinearTest *suite) { delete suite; }

  // instruments to test:
  // TOPAZ: no parameters file
  // EQSANS: no deltaE-mode parameter
  // HYSPEC: deltaE-mode='direct'
  // TOSCA: deltaE-mode='indirect', no Moderator.TimeZero parameters
  // BASIS: deltaE-mode='indirect', Moderator.TimeZero parameters found. Will
  // test event and histo files

  // Test several workspace inputs
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExecThrowsDeltaEmode() {
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    ModeratorTzeroLinear alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setRethrows(true); // necessary, otherwise the algorithm will catch all
                           // exceptions and not return them
    TS_ASSERT_THROWS(alg.execute(), Exception::InstrumentDefinitionError);
    AnalysisDataService::Instance().remove("testWS");
  }

  void testExecThrowsNoFormula() {
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    const bool add_deltaE_mode = true;
    addToInstrument(testWS, add_deltaE_mode);
    ModeratorTzeroLinear alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setRethrows(true); // necessary, otherwise the algorithm will catch all
                           // exceptions and not return them
    TS_ASSERT_THROWS(alg.execute(), Exception::InstrumentDefinitionError);
    AnalysisDataService::Instance().remove("testWS");
  }

  void testExecManualOverride() {
    // Workspace with indirect instrument
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    const bool add_deltaE_mode = true;
    addToInstrument(testWS, add_deltaE_mode);

    // Pass input parameters to the algorithm. Algorithm will execute
    // even though the instrument lacks parameter Gradient and Intercept
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("Gradient", 24.0);
    alg.setProperty("Intercept", 42.0);
    alg.setProperty("OutputWorkspace", "outWS1");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Add parameters to the instrument. Parameter values (11.0 and -5.0)
    // are different than the manual values (24.0 and 42.0)
    const bool add_t0_formula = true;
    addToInstrument(testWS, add_deltaE_mode, add_t0_formula);
    alg.setProperty("OutputWorkspace", "outWS2");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Instrument parameters are not used because the manual values override
    // Thus, TOFs in outWS2 should be the same as outWS1.
    // Note: instruments will be different
    auto checkAlg =
        AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    checkAlg->initialize();
    checkAlg->setChild(true);
    checkAlg->setProperty("Workspace1", "outWS1");
    checkAlg->setProperty("Workspace2", "outWS2");
    checkAlg->setProperty("CheckInstrument", false);
    checkAlg->setProperty("Tolerance", 1.0e-9);
    checkAlg->execute();
    TS_ASSERT(checkAlg->getProperty("Result"));

    AnalysisDataService::Instance().remove("testWS");
    AnalysisDataService::Instance().remove("outWS1");
    AnalysisDataService::Instance().remove("outWS2");
  }

  /*
   * First spectrum is a detector. Remaining two spectra are monitors
   */
  void testExecHistogram() {
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    const bool add_deltaE_mode = true;
    const bool add_t0_formula = true;
    addToInstrument(testWS, add_deltaE_mode, add_t0_formula);
    ModeratorTzeroLinear alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      auto &xarray = testWS->x(ihist);
      for (size_t ibin = 0; ibin < xarray.size(); ibin += 400)
        TS_ASSERT_DELTA(1600 * ibin / 400, xarray[ibin], 0.1);
    }
    AnalysisDataService::Instance().remove("testWS");
  }

  void testExecEvents() {
    EventWorkspace_sptr testWS = CreateEventWorkspace();
    const bool add_deltaE_mode = true;
    const bool add_t0_formula = true;
    addToInstrument(testWS, add_deltaE_mode, add_t0_formula);
    ModeratorTzeroLinear alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      const EventList &evlist = testWS->getSpectrum(ihist);
      const MantidVec &tofs_b = evlist.getTofs();
      auto &xarray = evlist.x();
      for (size_t ibin = 0; ibin < xarray.size(); ibin += 400) {
        TS_ASSERT_DELTA(1600 * ibin / 400, xarray[ibin], 0.1);
        TS_ASSERT_DELTA(1600 * ibin / 400, tofs_b[ibin], 0.2);
      }
    }
    AnalysisDataService::Instance().remove("testWS");
  }

private:
  MatrixWorkspace_sptr CreateHistogramWorkspace() {
    const int numHists(3);
    const int numBins(4000);
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            numHists, numBins, true);
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
    BinEdges xdata(numBins + 1, LinearGenerator(0.0, 4.0));
    const double peakHeight(1000), peakCentre(7000.), sigmaSq(1000 * 1000.);
    auto &Y = testWS->mutableY(0);
    for (int ibin = 0; ibin < numBins; ++ibin) {
      Y[ibin] =
          peakHeight * exp(-0.5 * pow(xdata[ibin] - peakCentre, 2.) / sigmaSq);
    }
    for (int ihist = 0; ihist < numHists; ihist++)
      testWS->setBinEdges(ihist, xdata);
    return testWS;
  }

  EventWorkspace_sptr CreateEventWorkspace() {
    const int numBanks(1), numPixels(1), numBins(4000);
    const bool clearEvents(true);
    EventWorkspace_sptr testWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
            numBanks, numPixels, clearEvents);
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
    const size_t numHists = testWS->getNumberHistograms();
    for (size_t ihist = 0; ihist < numHists; ++ihist) {
      EventList &evlist = testWS->getSpectrum(ihist);
      BinEdges xdata(numBins + 1, LinearGenerator(0.0, 4.0));
      for (auto &tof : xdata) {
        evlist.addEventQuickly(TofEvent(tof));
      }
      evlist.setX(xdata.cowData()); // set the bins for the associated histogram
    }
    return testWS;
  }

  ModeratorTzeroLinear alg;

}; // end of class ModeratorTzeroLinearTest : public CxxTest::TestSuite

class ModeratorTzeroLinearTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModeratorTzeroLinearTestPerformance *createSuite() {
    return new ModeratorTzeroLinearTestPerformance();
  }
  static void destroySuite(ModeratorTzeroLinearTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  ModeratorTzeroLinearTestPerformance() {
    input = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        10000, 1000, true);
    input->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");

    inputEvent =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 100,
                                                                        true);
    inputEvent->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
  }

  void testExec() {
    addToInstrument(input, true, true);
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.execute();
  }

  void testExecEvent() {
    addToInstrument(inputEvent, true, true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputEvent);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.execute();
  }

private:
  ModeratorTzeroLinear alg;
  MatrixWorkspace_sptr input;
  EventWorkspace_sptr inputEvent;
};

#endif /* MANTID_ALGORITHMS_MODERATORTZEROLINEARTEST_H_ */
