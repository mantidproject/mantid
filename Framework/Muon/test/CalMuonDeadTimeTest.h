// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CALMUONDEADTIMETEST_H_
#define CALMUONDEADTIMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMuon/CalMuonDeadTime.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CalMuonDeadTimeTest : public CxxTest::TestSuite {
public:
  void testName() {
    CalMuonDeadTime calDeadTime;
    TS_ASSERT_EQUALS(calDeadTime.name(), "CalMuonDeadTime")
  }

  void testCategory() {
    CalMuonDeadTime calDeadTime;
    TS_ASSERT_EQUALS(calDeadTime.category(), "Muon")
  }

  void testInit() {
    CalMuonDeadTime calDeadTime;
    calDeadTime.initialize();
    TS_ASSERT(calDeadTime.isInitialized())
  }

  void testCalDeadTime() {
    auto inputWS = loadData();

    CalMuonDeadTime calDeadTime;
    calDeadTime.initialize();
    calDeadTime.setProperty("InputWorkspace", inputWS);
    calDeadTime.setPropertyValue("DeadTimeTable", "deadtimetable");
    calDeadTime.setPropertyValue("DataFitted", "fittedData");
    calDeadTime.setPropertyValue("FirstGoodData", "1.0");
    calDeadTime.setPropertyValue("LastGoodData", "2.0");

    try {
      TS_ASSERT_EQUALS(calDeadTime.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

    ITableWorkspace_sptr table =
        boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "DeadTimeTable"));

    Column_const_sptr col = table->getColumn(1);
    const Column *tableC = col.get();
    TS_ASSERT_DELTA(tableC->operator[](0), -0.0238, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove("deadtimetable");
    Mantid::API::AnalysisDataService::Instance().remove("fittedData");
    Mantid::API::AnalysisDataService::Instance().remove("EMU6473");
  }

  void testNoGoodfrmPresent() {
    auto inputWS = loadData();

    auto &run = inputWS->mutableRun();
    run.removeLogData("goodfrm");
    TS_ASSERT(!run.hasProperty("goodfrm"));

    CalMuonDeadTime calDeadTime;
    calDeadTime.initialize();
    calDeadTime.setRethrows(true);
    calDeadTime.setProperty("InputWorkspace", inputWS);
    calDeadTime.setPropertyValue("DeadTimeTable", "deadtimetable");
    calDeadTime.setPropertyValue("DataFitted", "fittedData");
    calDeadTime.setPropertyValue("FirstGoodData", "1.0");
    calDeadTime.setPropertyValue("LastGoodData", "2.0");

    TS_ASSERT_THROWS(calDeadTime.execute(), std::runtime_error);
    TS_ASSERT(!calDeadTime.isExecuted());
  }

private:
  // Load the muon nexus file
  MatrixWorkspace_sptr loadData() {
    Mantid::DataHandling::LoadMuonNexus1 loader;
    loader.initialize();
    loader.setChild(true);
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "__NotUsed");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);
    Workspace_sptr outputWS = loader.getProperty("OutputWorkspace");
    return boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
  }
};

#endif /*CALMUONDEADTIMETEST_H_*/
