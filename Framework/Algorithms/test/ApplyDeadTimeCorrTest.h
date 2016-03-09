#ifndef MANTID_ALGORITHMS_APPLYDEADTIMECORRTEST_H_
#define MANTID_ALGORITHMS_APPLYDEADTIMECORRTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/ApplyDeadTimeCorr.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class ApplyDeadTimeCorrTest : public CxxTest::TestSuite {
public:
  void testName() {
    TS_ASSERT_EQUALS(applyDeadTime.name(), "ApplyDeadTimeCorr")
  }

  void testCategory() {
    TS_ASSERT_EQUALS(applyDeadTime.category(),
                     "Muon;CorrectionFunctions\\EfficiencyCorrections")
  }

  void testInit() {
    applyDeadTime.initialize();
    TS_ASSERT(applyDeadTime.isInitialized())
  }

  void testExec() {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    MatrixWorkspace_sptr inputWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473");

    boost::shared_ptr<ITableWorkspace> tw(
        new Mantid::DataObjects::TableWorkspace);

    tw->addColumn("int", "Spectrum Number");
    tw->addColumn("double", "DeadTime Value");

    // Add data to table

    double deadValue(-0.00456);

    for (int i = 0; i < 32; ++i) {
      Mantid::API::TableRow row = tw->appendRow();
      row << i + 1 << deadValue;
    }

    AnalysisDataService::Instance().add("DeadTimeTable", tw);

    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("InputWorkspace", "EMU6473"));
    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("DeadTimeTable", "DeadTimeTable"));
    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("OutputWorkspace", "AppliedTest"));

    TS_ASSERT_THROWS_NOTHING(applyDeadTime.execute());
    TS_ASSERT(applyDeadTime.isExecuted());

    double numGoodFrames = 1.0;
    const Run &run = inputWs->run();
    TS_ASSERT(run.hasProperty("goodfrm"))

    numGoodFrames =
        boost::lexical_cast<double>(run.getProperty("goodfrm")->value());

    MatrixWorkspace_sptr outputWs =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("AppliedTest"));

    TS_ASSERT_EQUALS(
        outputWs->dataY(0)[0],
        inputWs->dataY(0)[0] /
            (1 -
             inputWs->dataY(0)[0] *
                 (deadValue / ((inputWs->dataX(0)[1] - inputWs->dataX(0)[0]) *
                               numGoodFrames))));
    TS_ASSERT_EQUALS(
        outputWs->dataY(0)[40],
        inputWs->dataY(0)[40] /
            (1 -
             inputWs->dataY(0)[40] *
                 (deadValue / ((inputWs->dataX(0)[1] - inputWs->dataX(0)[0]) *
                               numGoodFrames))));
    TS_ASSERT_EQUALS(
        outputWs->dataY(31)[20],
        inputWs->dataY(31)[20] /
            (1 -
             inputWs->dataY(31)[20] *
                 (deadValue / ((inputWs->dataX(0)[1] - inputWs->dataX(0)[0]) *
                               numGoodFrames))));

    TS_ASSERT_DELTA(35.9991, outputWs->dataY(12)[2], 0.001);
    TS_ASSERT_DELTA(4901.5439, outputWs->dataY(20)[14], 0.001);

    AnalysisDataService::Instance().remove("EMU6473");
    AnalysisDataService::Instance().remove("DeadTimeTable");
    AnalysisDataService::Instance().remove("AppliedTest");
  }

  void testDifferentSize() {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    MatrixWorkspace_sptr inputWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473");

    boost::shared_ptr<ITableWorkspace> tw(
        new Mantid::DataObjects::TableWorkspace);

    tw->addColumn("int", "Spectrum Number");
    tw->addColumn("double", "DeadTime Value");

    // Add data to table

    const double deadValue(-0.00456);

    // Bigger row count than file (expect to fail)
    for (int i = 0; i < 64; ++i) {
      Mantid::API::TableRow row = tw->appendRow();
      row << i + 1 << deadValue;
    }

    AnalysisDataService::Instance().add("DeadTimeTable", tw);

    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("InputWorkspace", "EMU6473"));
    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("DeadTimeTable", "DeadTimeTable"));
    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("OutputWorkspace", "AppliedTest"));

    TS_ASSERT_THROWS_NOTHING(applyDeadTime.execute());
    TS_ASSERT(applyDeadTime.isExecuted());

    // Check new table wasn't created
    TS_ASSERT(!(AnalysisDataService::Instance().doesExist("AppliedTest")));

    AnalysisDataService::Instance().remove("EMU6473");
    AnalysisDataService::Instance().remove("DeadTimeTable");
  }

  void testSelectedSpectrum() {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    MatrixWorkspace_sptr inputWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EMU6473");

    boost::shared_ptr<ITableWorkspace> tw(
        new Mantid::DataObjects::TableWorkspace);

    tw->addColumn("int", "Spectrum Number");
    tw->addColumn("double", "DeadTime Value");

    // Add data to table

    const double deadValue(-0.00456);

    // Spectrum: 3,6,9,12,15,18,21 .....
    for (int i = 0; i < 7; ++i) {
      Mantid::API::TableRow row = tw->appendRow();
      row << (i + 1) * 3 << deadValue;
    }

    //.... Index will therefore be 2,5,8,11,14,17,20

    AnalysisDataService::Instance().add("DeadTimeTable", tw);

    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("InputWorkspace", "EMU6473"));
    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("DeadTimeTable", "DeadTimeTable"));
    TS_ASSERT_THROWS_NOTHING(
        applyDeadTime.setProperty("OutputWorkspace", "AppliedTest"));

    TS_ASSERT_THROWS_NOTHING(applyDeadTime.execute());
    TS_ASSERT(applyDeadTime.isExecuted());

    double numGoodFrames = 1.0;
    const Run &run = inputWs->run();
    TS_ASSERT(run.hasProperty("goodfrm"))

    numGoodFrames =
        boost::lexical_cast<double>(run.getProperty("goodfrm")->value());

    MatrixWorkspace_sptr outputWs =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("AppliedTest"));

    TS_ASSERT_EQUALS(outputWs->dataY(0)[0], inputWs->dataY(0)[0]);
    TS_ASSERT_EQUALS(
        outputWs->dataY(14)[40],
        inputWs->dataY(14)[40] /
            (1 -
             inputWs->dataY(14)[40] *
                 (deadValue / ((inputWs->dataX(0)[1] - inputWs->dataX(0)[0]) *
                               numGoodFrames))));
    TS_ASSERT_EQUALS(outputWs->dataY(31)[20], inputWs->dataY(31)[20]);

    // Should be the same (no dead time associated with it)
    TS_ASSERT_DELTA(36.0, outputWs->dataY(12)[2], 0.1);

    // Should be new value (dead time applied based on spectrum number)
    TS_ASSERT_DELTA(4901.5439, outputWs->dataY(20)[14], 0.001);

    AnalysisDataService::Instance().remove("EMU6473");
    AnalysisDataService::Instance().remove("DeadTimeTable");
    AnalysisDataService::Instance().remove("AppliedTest");
  }

  /// Test algorithm rejects an input workspace with uneven bin widths
  void testUnevenBinWidths() {
    constexpr size_t numSpectra(2);
    auto workspace = WorkspaceCreationHelper::Create2DWorkspace(
        static_cast<int>(numSpectra), 10);

    // Rebin the workspace to make bin widths uneven
    auto rebin = AlgorithmFactory::Instance().create("Rebin", 1);
    rebin->initialize();
    rebin->setChild(true);
    rebin->setProperty("InputWorkspace", workspace);
    rebin->setPropertyValue("OutputWorkspace", "__NotUsed");
    rebin->setPropertyValue("Params", "0, 3, 6, 1, 10"); // uneven bins
    rebin->execute();
    MatrixWorkspace_sptr rebinned = rebin->getProperty("OutputWorkspace");

    // Dead time table
    auto deadTimes = makeDeadTimeTable(numSpectra);

    // Test that algorithm throws
    ApplyDeadTimeCorr applyDT;
    applyDT.initialize();
    applyDT.setChild(true);
    TS_ASSERT_THROWS_NOTHING(applyDT.setProperty("InputWorkspace", rebinned));
    applyDT.setProperty("DeadTimeTable", deadTimes);
    applyDT.setProperty("OutputWorkspace", "__NotUsed");
    TS_ASSERT_THROWS(applyDT.execute(), std::runtime_error);
  }

private:
  /**
   * Generates a dead time table with the given number of spectra
   * @param numSpectra :: [input] Number of rows in the table
   * @returns :: Dead time table
   */
  boost::shared_ptr<ITableWorkspace> makeDeadTimeTable(size_t numSpectra) {
    constexpr static double deadValue(-0.00456);
    boost::shared_ptr<ITableWorkspace> deadTimes =
        boost::make_shared<Mantid::DataObjects::TableWorkspace>();
    deadTimes->addColumn("int", "Spectrum Number");
    deadTimes->addColumn("double", "DeadTime Value");
    for (size_t i = 0; i < numSpectra; i++) {
      Mantid::API::TableRow row = deadTimes->appendRow();
      row << static_cast<int>(i + 1) << deadValue;
    }
    return deadTimes;
  }

  ApplyDeadTimeCorr applyDeadTime;
  Mantid::DataHandling::LoadMuonNexus2 loader;
};

#endif /* MANTID_ALGORITHMS_APPLYDEADTIMECORRTEST_H_ */
