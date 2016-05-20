#ifndef MANTID_ALGORITHMS_MODERATORTZEROLINEARTEST_H_
#define MANTID_ALGORITHMS_MODERATORTZEROLINEARTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ModeratorTzeroLinear.h"
#include "MantidAPI/Axis.h"
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;

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
    AddToInstrument(testWS, add_deltaE_mode);
    ModeratorTzeroLinear alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setRethrows(true); // necessary, otherwise the algorithm will catch all
                           // exceptions and not return them
    TS_ASSERT_THROWS(alg.execute(), Exception::InstrumentDefinitionError);
    AnalysisDataService::Instance().remove("testWS");
  }

  /*
   * First spectrum is a detector. Remaining two spectra are monitors
   */
  void testExecHistogram() {
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    const bool add_deltaE_mode = true;
    const bool add_t0_formula = true;
    AddToInstrument(testWS, add_deltaE_mode, add_t0_formula);
    ModeratorTzeroLinear alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      const MantidVec &xarray = testWS->readX(ihist);
      for (size_t ibin = 0; ibin < xarray.size(); ibin += 400)
        TS_ASSERT_DELTA(1600 * ibin / 400, xarray[ibin], 0.1);
    }
    AnalysisDataService::Instance().remove("testWS");
  }

  void testExecEvents() {
    EventWorkspace_sptr testWS = CreateEventWorkspace();
    const bool add_deltaE_mode = true;
    const bool add_t0_formula = true;
    AddToInstrument(testWS, add_deltaE_mode, add_t0_formula);
    ModeratorTzeroLinear alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      const EventList &evlist = testWS->getEventList(ihist);
      const MantidVec &tofs_b = evlist.getTofs();
      const MantidVec &xarray = evlist.readX();
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
    BinEdges xdata(numBins + 1);
    const double peakHeight(1000), peakCentre(7000.), sigmaSq(1000 * 1000.);
    for (int ibin = 0; ibin < numBins; ++ibin) {
      const double xValue = 4 * ibin;
      testWS->dataY(0)[ibin] =
          peakHeight * exp(-0.5 * pow(xValue - peakCentre, 2.) / sigmaSq);
      xdata.mutableData()[ibin] = xValue;
    }
    xdata.mutableData()[numBins] = 4 * numBins;
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
      EventList &evlist = testWS->getEventList(ihist);
      BinEdges xdata(numBins + 1);
      for (int ibin = 0; ibin <= numBins; ++ibin) {
        double tof = 4 * ibin;
        TofEvent tofevent(tof);
        xdata.mutableData()[ibin] = tof;
        evlist.addEventQuickly(tofevent); // insert event
      }
      evlist.setX(xdata.cowData()); // set the bins for the associated histogram
    }
    return testWS;
  }

  void AddToInstrument(MatrixWorkspace_sptr testWS,
                       const bool &add_deltaE_mode = false,
                       const bool &add_t0_formula = false) {
    const double evalue(2.082); // energy corresponding to the first order Bragg
                                // peak in the analyzers
    if (add_deltaE_mode) {
      testWS->instrumentParameters().addString(
          testWS->getInstrument()->getComponentID(), "deltaE-mode", "indirect");
      for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist)
        testWS->instrumentParameters().addDouble(
            testWS->getDetector(ihist)->getComponentID(), "Efixed", evalue);
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

  ModeratorTzeroLinear alg;

}; // end of class ModeratorTzeroLinearTest : public CxxTest::TestSuite

#endif /* MANTID_ALGORITHMS_MODERATORTZEROLINEARTEST_H_ */
