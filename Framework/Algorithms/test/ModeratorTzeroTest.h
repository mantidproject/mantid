#ifndef MANTID_ALGORITHMS_MODERATORTZEROTEST_H_
#define MANTID_ALGORITHMS_MODERATORTZEROTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/ModeratorTzero.h"
#include "MantidDataObjects/Events.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Algorithms;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;
using Mantid::Types::Event::TofEvent;

namespace {
void AddToIndirectInstrument(MatrixWorkspace_sptr &testWS,
                             const bool &add_t0_formula = false,
                             const bool &add_Efixed = false) {

  if (add_t0_formula)
    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getComponentID(), "t0_formula",
        "50.-(50./52500)*incidentEnergy");

  if (add_Efixed) {
    const double evalue(2.082); // energy corresponding to the first order
                                // Bragg peak in the analyzers
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      testWS->instrumentParameters().addDouble(
          testWS->getDetector(ihist)->getComponentID(), "Efixed", evalue);
    }
  }
} // end of void AddToInstrument
} // namespace

class ModeratorTzeroTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModeratorTzeroTest *createSuite() { return new ModeratorTzeroTest(); }
  static void destroySuite(ModeratorTzeroTest *suite) { delete suite; }

  ModeratorTzeroTest() {
    FrameworkManager::Instance(); // Load plugins
  }

  void TestInit() {
    ModeratorTzero alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void TestExecHistogramIndirect() {
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    const bool add_Efixed = true;
    const bool add_t0_formula = true;
    AddToIndirectInstrument(testWS, add_t0_formula, add_Efixed);
    ModeratorTzero alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setProperty("EMode", "Indirect");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values. These are values separated by 400 bins
    const size_t jump(400);
    double tofs[3][11] = {{-0.218694, 1599.78, 3199.78, 4799.78, 6399.78,
                           7999.78, 9550.71, 11150.2, 12750.1, 14350, 15950},
                          {-34.9412, 1550.24, 3150.06, 4750.03, 6350.01,
                           7950.01, 9550.01, 11150, 12750, 14350, 15950},
                          {-9.67714, 1550.63, 3150.16, 4750.07, 6350.04,
                           7950.03, 9550.02, 11150, 12750, 14350, 15950}};
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      auto &xarray = testWS->x(ihist);
      for (size_t ibin = 0; ibin < xarray.size(); ibin += jump) {
        TS_ASSERT_DELTA(tofs[ihist][ibin / jump], xarray[ibin], 0.1);
      }
    }
    AnalysisDataService::Instance().remove("testWS");
  }

  void TestExecHistogramElastic() {
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getComponentID(), "t0_formula",
        "101.9*incidentEnergy^(-0.41)*exp(-incidentEnergy/282.0)");
    ModeratorTzero alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setProperty("EMode", "Elastic");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values. These are values separated by 400 bins
    const size_t jump(400);
    double tofs[3][11] = {{0.0, 1599.94, 3196.91, 4791.92, 6387.25, 7983.04,
                           9579.19, 11175.6, 12772.2, 14368.9, 15965.7},
                          {0.0, 1595.57, 3184.9, 4776.23, 6368.59, 7961.54,
                           9554.85, 11148.4, 12742.2, 14336.2, 15930.3},
                          {0.0, 1599.32, 3193.02, 4786.52, 6380.87, 7975.78,
                           9571.06, 11166.6, 12762.3, 14358.2, 15954.1}};
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      auto &xarray = testWS->x(ihist);
      for (size_t ibin = 0; ibin < xarray.size(); ibin += jump) {
        TS_ASSERT_DELTA(tofs[ihist][ibin / jump], xarray[ibin], 0.1);
      }
    }
    AnalysisDataService::Instance().remove("testWS");
  }

  void TestExecHistogramDirect() {
    MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getComponentID(), "t0_formula",
        "101.9*incidentEnergy^(-0.41)*exp(-incidentEnergy/282.0)");
    testWS->mutableRun().addProperty("Ei", 100.0, "meV", true);
    ModeratorTzero alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setProperty("EMode", "Direct");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values. These are values separated by 400 bins
    const size_t jump(400);
    double tofs[3][11] = {
        {-10.8185, 1589.18, 3189.18, 4789.18, 6389.18, 7989.18, 9589.18,
         11189.2, 12789.2, 14389.2, 15989.2},
        {-10.8185, 1589.18, 3189.18, 4789.18, 6389.18, 7989.18, 9589.18,
         11189.2, 12789.2, 14389.2, 15989.2},
        {-10.8185, 1589.18, 3189.18, 4789.18, 6389.18, 7989.18, 9589.18,
         11189.2, 12789.2, 14389.2, 15989.2}};
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      auto &xarray = testWS->x(ihist);
      for (size_t ibin = 0; ibin < xarray.size(); ibin += jump) {
        TS_ASSERT_DELTA(tofs[ihist][ibin / jump], xarray[ibin], 0.1);
      }
    }
    AnalysisDataService::Instance().remove("testWS");
  }

  void TestExecEventsIndirect() {
    EventWorkspace_sptr testWS = CreateEventWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    const bool add_Efixed = true;
    const bool add_t0_formula = true;
    MatrixWorkspace_sptr mtestWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(testWS);
    AddToIndirectInstrument(mtestWS, add_t0_formula, add_Efixed);
    ModeratorTzero alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setProperty("EMode", "Indirect");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values.  These are values separated by 400 bins
    const size_t jump(400);
    double tofs_a[11] = {-37.5547, 1562.45, 3162.45, 4762.45, 6362.45, 7962.45,
                         9550.18,  11150,   12750,   14350,   15950};
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      EventList &evlist = testWS->getSpectrum(ihist);
      MantidVec tofs_b = evlist.getTofs();
      auto &xarray = evlist.x();
      for (size_t ibin = 0; ibin < xarray.size(); ibin += jump) {
        TS_ASSERT_DELTA(tofs_a[ibin / jump], xarray[ibin], 0.1);
        TS_ASSERT_DELTA(tofs_a[ibin / jump], tofs_b[ibin], 0.2);
      }
    }
    AnalysisDataService::Instance().remove("testWS");
  }

  void TestExecEventsElastic() {
    EventWorkspace_sptr testWS = CreateEventWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    MatrixWorkspace_sptr mtestWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(testWS);
    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getComponentID(), "t0_formula",
        "101.9*incidentEnergy^(-0.41)*exp(-incidentEnergy/282.0)");
    ModeratorTzero alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setProperty("EMode", "Elastic");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values.  These are values separated by 400 bins
    const size_t jump(400);
    double tofs_a[11] = {0.0,     1598.38, 3190.3,  4783.04, 6376.76, 7971.06,
                         9565.72, 11160.6, 12755.7, 14351,   15946.3};
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      EventList &evlist = testWS->getSpectrum(ihist);
      MantidVec tofs_b = evlist.getTofs();
      auto &xarray = evlist.x();
      for (size_t ibin = 0; ibin < xarray.size(); ibin += jump) {
        TS_ASSERT_DELTA(tofs_a[ibin / jump], xarray[ibin], 0.1);
        TS_ASSERT_DELTA(tofs_a[ibin / jump], tofs_b[ibin], 0.2);
      }
    }
    AnalysisDataService::Instance().remove("testWS");
  }

  void TestExecEventsDirect() {
    EventWorkspace_sptr testWS = CreateEventWorkspace();
    AnalysisDataService::Instance().add("testWS", testWS);
    MatrixWorkspace_sptr mtestWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(testWS);
    testWS->instrumentParameters().addString(
        testWS->getInstrument()->getComponentID(), "t0_formula",
        "101.9*incidentEnergy^(-0.41)*exp(-incidentEnergy/282.0)");
    testWS->mutableRun().addProperty("Ei", 100.0, "meV", true);
    ModeratorTzero alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", testWS);
    alg.setProperty("OutputWorkspace", "testWS");
    alg.setProperty("EMode", "Direct");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check a few values.  These are values separated by 400 bins
    const size_t jump(400);
    double tofs_a[11] = {-10.8185, 1589.18, 3189.18, 4789.18, 6389.18, 7989.18,
                         9589.18,  11189.2, 12789.2, 14389.2, 15989.2};
    for (size_t ihist = 0; ihist < testWS->getNumberHistograms(); ++ihist) {
      EventList &evlist = testWS->getSpectrum(ihist);
      MantidVec tofs_b = evlist.getTofs();
      auto &xarray = evlist.x();
      for (size_t ibin = 0; ibin < xarray.size(); ibin += jump) {
        TS_ASSERT_DELTA(tofs_a[ibin / jump], xarray[ibin], 0.1);
        TS_ASSERT_DELTA(tofs_a[ibin / jump], tofs_b[ibin], 0.2);
      }
    }
    AnalysisDataService::Instance().remove("testWS");
  }

private:
  /*
   * First spectrum is a detector. Remaining two spectra are monitors
   * Detector contains histogram of tof with a Gaussian profile
   */
  MatrixWorkspace_sptr CreateHistogramWorkspace() {
    const int numHists(3);
    const int numBins(4000);
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            numHists, numBins, true);
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
    BinEdges xdata(numBins + 1, LinearGenerator(0.0, 4.0));
    const double peakHeight(1000.), peakCentre(7000.), sigmaSq(1000 * 1000.);

    auto &Y = testWS->mutableY(0);
    // tof ranges from 0 to 16000 (units assumed micro-seconds
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
    const double rescaling_factor(4.0);
    const size_t numHists = testWS->getNumberHistograms();
    for (size_t ihist = 0; ihist < numHists; ++ihist) {
      EventList &evlist = testWS->getSpectrum(ihist);
      BinEdges xdata(numBins + 1, LinearGenerator(0.0, rescaling_factor));

      for (auto &tof : xdata)
        evlist.addEventQuickly(TofEvent(tof));

      evlist.setX(xdata.cowData()); // set the bins for the associated histogram
    }
    return testWS;
  }
};

class ModeratorTzeroTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModeratorTzeroTestPerformance *createSuite() {
    return new ModeratorTzeroTestPerformance();
  }
  static void destroySuite(ModeratorTzeroTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  ModeratorTzeroTestPerformance() {
    input = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        10000, 1000, true);
    input->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");

    output = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        10000, 1000, true);
    output->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");

    inputEvent =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 100,
                                                                        true);
    inputEvent->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");

    outputEvent =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 100,
                                                                        true);
    outputEvent->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");

    test = boost::dynamic_pointer_cast<MatrixWorkspace>(inputEvent);
    AnalysisDataService::Instance().add("input", input);
    AnalysisDataService::Instance().add("output", output);
    AnalysisDataService::Instance().add("inputEvent", inputEvent);
    AnalysisDataService::Instance().add("outputEvent", outputEvent);
  }

  ~ModeratorTzeroTestPerformance() {
    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove("output");
    AnalysisDataService::Instance().remove("inputEvent");
    AnalysisDataService::Instance().remove("outputEvent");
  }

  void testExecIndirect() {
    AddToIndirectInstrument(input, true, true);
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setProperty("EMode", "Indirect");
    alg.execute();
  }

  void testExecIndirectEvent() {
    AddToIndirectInstrument(test, true, true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputEvent);
    alg.setPropertyValue("OutputWorkspace", "outputEvent");
    alg.setProperty("EMode", "Indirect");
    alg.execute();
  }

  void testExecDirect() {
    input->instrumentParameters().addString(
        input->getInstrument()->getComponentID(), "t0_formula",
        "101.9*incidentEnergy^(-0.41)*exp(-incidentEnergy/282.0)");
    input->mutableRun().addProperty("Ei", 100.0, "meV", true);

    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setProperty("EMode", "Direct");
    alg.execute();
  }

  void testExecDirectEvent() {
    inputEvent->instrumentParameters().addString(
        inputEvent->getInstrument()->getComponentID(), "t0_formula",
        "101.9*incidentEnergy^(-0.41)*exp(-incidentEnergy/282.0)");
    inputEvent->mutableRun().addProperty("Ei", 100.0, "meV", true);

    alg.initialize();
    alg.setProperty("InputWorkspace", inputEvent);
    alg.setPropertyValue("OutputWorkspace", "outputEvent");
    alg.setProperty("EMode", "Direct");
    alg.execute();
  }

private:
  ModeratorTzero alg;
  MatrixWorkspace_sptr input;
  MatrixWorkspace_sptr test;
  EventWorkspace_sptr inputEvent;
  MatrixWorkspace_sptr output;
  EventWorkspace_sptr outputEvent;
};

#endif /*MANTID_ALGORITHMS_MODERATORTZEROTEST_H_*/
