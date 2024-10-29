// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMuon/ApplyDeadTimeCorr.h"
#include "MantidMuon/LoadMuonNexus2.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class ApplyDeadTimeCorrTest : public CxxTest::TestSuite {
public:
  void testName() {
    ApplyDeadTimeCorr applyDeadTime;
    TS_ASSERT_EQUALS(applyDeadTime.name(), "ApplyDeadTimeCorr")
  }

  void testCategory() {
    ApplyDeadTimeCorr applyDeadTime;
    TS_ASSERT_EQUALS(applyDeadTime.category(), "Muon;CorrectionFunctions\\EfficiencyCorrections")
  }

  void testInit() {
    ApplyDeadTimeCorr applyDeadTime;
    applyDeadTime.initialize();
    TS_ASSERT(applyDeadTime.isInitialized())
  }

  void testExec() {
    MatrixWorkspace_sptr inputWs = loadDataFromFile();
    auto deadTimes = makeDeadTimeTable(32);

    ApplyDeadTimeCorr applyDeadTime;
    applyDeadTime.initialize();
    applyDeadTime.setChild(true);
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("DeadTimeTable", deadTimes));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("OutputWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.execute());
    TS_ASSERT(applyDeadTime.isExecuted());

    double numGoodFrames = 1.0;
    const Run &run = inputWs->run();
    TS_ASSERT(run.hasProperty("goodfrm"))

    numGoodFrames = boost::lexical_cast<double>(run.getProperty("goodfrm")->value());

    MatrixWorkspace_sptr outputWs = applyDeadTime.getProperty("OutputWorkspace");
    TS_ASSERT(outputWs);

    TS_ASSERT_EQUALS(outputWs->y(0)[0],
                     inputWs->y(0)[0] / (1 - inputWs->y(0)[0] * (deadValue() / ((inputWs->x(0)[1] - inputWs->x(0)[0]) *
                                                                                numGoodFrames))));
    TS_ASSERT_EQUALS(
        outputWs->y(0)[40],
        inputWs->y(0)[40] /
            (1 - inputWs->y(0)[40] * (deadValue() / ((inputWs->x(0)[1] - inputWs->x(0)[0]) * numGoodFrames))));
    TS_ASSERT_EQUALS(
        outputWs->y(31)[20],
        inputWs->y(31)[20] /
            (1 - inputWs->y(31)[20] * (deadValue() / ((inputWs->x(0)[1] - inputWs->x(0)[0]) * numGoodFrames))));

    TS_ASSERT_DELTA(35.9991, outputWs->y(12)[2], 0.001);
    TS_ASSERT_DELTA(4901.5439, outputWs->y(20)[14], 0.001);
  }

  void testDifferentSize() {
    MatrixWorkspace_sptr inputWs = loadDataFromFile();

    // Bigger row count than file (expect to fail)
    auto deadTimes = makeDeadTimeTable(64);

    ApplyDeadTimeCorr applyDeadTime;
    applyDeadTime.initialize();
    applyDeadTime.setChild(true);
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("DeadTimeTable", deadTimes));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("OutputWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS(applyDeadTime.execute(), const std::logic_error &);

    // Check new table wasn't created
    MatrixWorkspace_sptr output = applyDeadTime.getProperty("OutputWorkspace");
    TS_ASSERT(!output);
  }

  void testSelectedSpectrum() {
    MatrixWorkspace_sptr inputWs = loadDataFromFile();

    std::shared_ptr<ITableWorkspace> deadTimes = std::make_shared<Mantid::DataObjects::TableWorkspace>();
    deadTimes->addColumn("int", "Spectrum Number");
    deadTimes->addColumn("double", "DeadTime Value");

    // Spectrum: 3,6,9,12,15,18,21 .....
    for (int i = 0; i < 7; ++i) {
      Mantid::API::TableRow row = deadTimes->appendRow();
      row << (i + 1) * 3 << deadValue();
    }

    //.... Index will therefore be 2,5,8,11,14,17,20

    ApplyDeadTimeCorr applyDeadTime;
    applyDeadTime.initialize();
    applyDeadTime.setChild(true);
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("DeadTimeTable", deadTimes));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("OutputWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.execute());
    TS_ASSERT(applyDeadTime.isExecuted());

    double numGoodFrames = 1.0;
    const Run &run = inputWs->run();
    TS_ASSERT(run.hasProperty("goodfrm"))

    numGoodFrames = boost::lexical_cast<double>(run.getProperty("goodfrm")->value());

    MatrixWorkspace_sptr outputWs = applyDeadTime.getProperty("OutputWorkspace");
    TS_ASSERT(outputWs);

    TS_ASSERT_EQUALS(outputWs->y(0)[0], inputWs->y(0)[0]);
    TS_ASSERT_EQUALS(
        outputWs->y(14)[40],
        inputWs->y(14)[40] /
            (1 - inputWs->y(14)[40] * (deadValue() / ((inputWs->x(0)[1] - inputWs->x(0)[0]) * numGoodFrames))));
    TS_ASSERT_EQUALS(outputWs->y(31)[20], inputWs->y(31)[20]);

    // Should be the same (no dead time associated with it)
    TS_ASSERT_DELTA(36.0, outputWs->y(12)[2], 0.1);

    // Should be new value (dead time applied based on spectrum number)
    TS_ASSERT_DELTA(4901.5439, outputWs->y(20)[14], 0.001);
  }

  /// Test algorithm rejects an input workspace with uneven bin widths
  void testUnevenBinWidths() {
    constexpr size_t numSpectra(2);
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(static_cast<int>(numSpectra), 10);

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

    // Test that algorithm throws when property is set
    ApplyDeadTimeCorr applyDT;
    applyDT.initialize();
    applyDT.setChild(true);
    TS_ASSERT_THROWS(applyDT.setProperty("InputWorkspace", rebinned), const std::invalid_argument &);
  }

  // Test that algorithm throws if input workspace does not contain number of
  // good frames
  void testNoGoodfrmPresent() {
    MatrixWorkspace_sptr inputWs = loadDataFromFile();
    auto deadTimes = makeDeadTimeTable(32);

    auto &run = inputWs->mutableRun();
    run.removeLogData("goodfrm");
    TS_ASSERT(!run.hasProperty("goodfrm"));

    ApplyDeadTimeCorr applyDeadTime;
    applyDeadTime.initialize();
    applyDeadTime.setChild(true);
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("DeadTimeTable", deadTimes));
    TS_ASSERT_THROWS_NOTHING(applyDeadTime.setProperty("OutputWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS(applyDeadTime.execute(), const std::invalid_argument &);
    TS_ASSERT(!applyDeadTime.isExecuted());
  }

private:
  /**
   * Generates a dead time table with the given number of spectra
   * @param numSpectra :: [input] Number of rows in the table
   * @returns :: Dead time table
   */
  std::shared_ptr<ITableWorkspace> makeDeadTimeTable(size_t numSpectra) {
    std::shared_ptr<ITableWorkspace> deadTimes = std::make_shared<Mantid::DataObjects::TableWorkspace>();
    deadTimes->addColumn("int", "Spectrum Number");
    deadTimes->addColumn("double", "DeadTime Value");
    for (size_t i = 0; i < numSpectra; i++) {
      Mantid::API::TableRow row = deadTimes->appendRow();
      row << static_cast<int>(i + 1) << deadValue();
    }
    return deadTimes;
  }

  /**
   * Loads data from the test data file
   * @returns :: Workspace with loaded data
   */
  MatrixWorkspace_sptr loadDataFromFile() {
    Mantid::Algorithms::LoadMuonNexus2 loader;
    loader.initialize();
    loader.setChild(true);
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "__NotUsed");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);
    Workspace_sptr data = loader.getProperty("OutputWorkspace");
    auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(data);
    TS_ASSERT(data);
    return matrixWS;
  }

  /// Test dead time value
  double deadValue() const { return -0.00456; }
};
