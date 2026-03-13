// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadDiffCal.h"
#include "MantidDataHandling/SaveDiffCal.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

using Mantid::DataHandling::SaveDiffCal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;

namespace {
const size_t NUM_BANK = 5;
const std::string FILENAME = "SaveDiffCalTest.h5";
const double DIFC_OFFSET = 2000.0;
} // namespace

class SaveDiffCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveDiffCalTest *createSuite() { return new SaveDiffCalTest(); }
  static void destroySuite(SaveDiffCalTest *suite) { delete suite; }

  void tearDown() override {
    // cleanup
    if (std::filesystem::exists(FILENAME)) {
      std::filesystem::remove(FILENAME);
    }
  }

  void test_Init() {
    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  Instrument_sptr createInstrument() { return ComponentCreationHelper::createTestInstrumentCylindrical(NUM_BANK); }

  GroupingWorkspace_sptr createGrouping(Instrument_sptr instr, bool single = true) {
    GroupingWorkspace_sptr groupWS = std::make_shared<GroupingWorkspace>(instr);
    if (single) {
      // set all of the groups to one
      size_t numHist = groupWS->getNumberHistograms();
      for (size_t i = 0; i < numHist; ++i) {
        const auto &detIds = groupWS->getDetectorIDs(i);
        groupWS->setValue(*(detIds.begin()), 1);
      }
    } else {
      groupWS->setValue(1, 12);
      groupWS->setValue(2, 23);
      groupWS->setValue(3, 45);
    }
    return groupWS;
  }

  MaskWorkspace_sptr createMasking(Instrument_sptr instr) {
    MaskWorkspace_sptr maskWS = std::make_shared<MaskWorkspace>(instr);
    maskWS->getSpectrum(0).clearData();
    maskWS->mutableSpectrumInfo().setMasked(0, true);
    return maskWS;
  }

  TableWorkspace_sptr createCalibration(const size_t numRows, const bool repeatEntries = false) {
    TableWorkspace_sptr wksp = std::make_shared<TableWorkspace>();
    wksp->addColumn("int", "detid");
    wksp->addColumn("double", "difc");
    wksp->addColumn("double", "difa");
    wksp->addColumn("double", "tzero");
    wksp->addColumn("double", "tofmin");

    const size_t tableSize = numRows * (1 + static_cast<int>(repeatEntries));

    for (size_t i = 0; i < tableSize; ++i) {
      TableRow row = wksp->appendRow();
      // To compare other than zeros later
      double difc = static_cast<double>(repeatEntries) * (DIFC_OFFSET + static_cast<double>(i % numRows));

      row << static_cast<int>(i % numRows) // detid
          << difc                          // difc
          << 0.                            // difa
          << 0.                            // tzero
          << 0.;                           // tofmin
    }
    return wksp;
  }

  void test_no_wksp() {
    // test that not supplying any workspaces is a problem
    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_no_cal_wksp() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);

    executeAndAssertAlgorithm(std::nullopt, groupWS, maskWS);
  }

  void test_empty_cal_wksp() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);
    TableWorkspace_sptr calWS = createCalibration(0);

    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalibrationWorkspace", calWS));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_no_mask() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    TableWorkspace_sptr calWS = createCalibration(NUM_BANK * 9); // nine components per bank

    executeAndAssertAlgorithm(calWS, groupWS);
  }

  void test_no_grouping() {
    Instrument_sptr inst = createInstrument();
    MaskWorkspace_sptr maskWS = createMasking(inst);
    TableWorkspace_sptr calWS = createCalibration(NUM_BANK * 9); // nine components per bank

    executeAndAssertAlgorithm(calWS, std::nullopt, maskWS);
  }

  void test_all_inputs() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);
    TableWorkspace_sptr calWS = createCalibration(NUM_BANK * 9); // nine components per bank

    executeAndAssertAlgorithm(calWS, groupWS, maskWS);
  }

  void test_calibration_ws_repeated_entries() {
    // Should be rare that this is the case, but it has happened: mantid #40853
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst, false);
    MaskWorkspace_sptr maskWS = createMasking(inst);

    TableWorkspace_sptr calWS = createCalibration(NUM_BANK * 9, true);

    executeAndAssertAlgorithm(calWS, groupWS, maskWS);
    checkLoadedDiffFileAgainstCalibrationTable(FILENAME, calWS);
  }

private:
  void executeAndAssertAlgorithm(const std::optional<ITableWorkspace_sptr> &tableWS = std::nullopt,
                                 const std::optional<GroupingWorkspace_sptr> &groupWS = std::nullopt,
                                 const std::optional<MaskWorkspace_sptr> &maskWS = std::nullopt) {

    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    if (tableWS.has_value()) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalibrationWorkspace", *tableWS));
    }
    if (groupWS.has_value()) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", *groupWS));
    }
    if (maskWS.has_value()) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", *maskWS));
    }
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // confirm that it exists
    const std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(std::filesystem::exists(filename));
  }
  void checkLoadedDiffFileAgainstCalibrationTable(const std::string &filename, const ITableWorkspace_sptr &calTable) {
    LoadDiffCal alg;

    alg.initialize();
    alg.setProperty("Filename", filename);
    alg.setProperty("WorkspaceName", "test");
    alg.setProperty("MakeMaskWorkspace", false);
    alg.setProperty("MakeGroupingWorkspace", false);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    const auto &ads = AnalysisDataService::Instance();

    TS_ASSERT(ads.doesExist("test_cal"));
    const auto loadedCalTable = ads.retrieveWS<ITableWorkspace>("test_cal");

    const auto difcCol = calTable->getColumn("difc");
    const auto loadedDifcCol = loadedCalTable->getColumn("difc");
    const auto difc = difcCol->numeric_fill<int32_t>();
    const auto loadedDifc = loadedDifcCol->numeric_fill<int32_t>();
    TS_ASSERT_EQUALS(difc, loadedDifc);

    const auto detidCol = calTable->getColumn("detid");
    const auto loadedDetidCol = loadedCalTable->getColumn("detid");
    const auto detID = detidCol->numeric_fill<int32_t>();
    const auto loadedDetID = loadedDetidCol->numeric_fill<int32_t>();
    TS_ASSERT_EQUALS(detID, loadedDetID);
  }
};
