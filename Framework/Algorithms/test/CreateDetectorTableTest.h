// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidAlgorithms/CreateDetectorTable.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Detector.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class CreateDetectorTableTest : public CxxTest::TestSuite {
public:
  static CreateDetectorTableTest *createSuite() { return new CreateDetectorTableTest(); }

  static void destroySuite(CreateDetectorTableTest *suite) { delete suite; }

  void test_Name() {
    CreateDetectorTable alg;
    TS_ASSERT_EQUALS(alg.name(), "CreateDetectorTable");
  }

  void test_Version() {
    Mantid::Algorithms::CreateDetectorTable alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_Init() {
    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    const auto &props = alg.getProperties();
    TS_ASSERT_EQUALS(props.size(), 6);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());

    TS_ASSERT_EQUALS(props[1]->name(), "WorkspaceIndices");
    TS_ASSERT(props[1]->isDefault());

    TS_ASSERT_EQUALS(props[2]->name(), "IncludeData");
    TS_ASSERT(props[2]->isDefault());

    TS_ASSERT_EQUALS(props[3]->name(), "IncludeDetectorPosition");
    TS_ASSERT(props[3]->isDefault());

    TS_ASSERT_EQUALS(props[4]->name(), "OneRowPerDetectorID");
    TS_ASSERT(props[4]->isDefault());

    TS_ASSERT_EQUALS(props[5]->name(), "DetectorTableWorkspace");
    TS_ASSERT(props[5]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<TableWorkspace> *>(props[5]));
  }

  void test_Exec_Matrix_Workspace() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 11);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), 1); // Spectrum No should be 1, if not in the exception
  }

  void test_Exec_Matrix_Workspace_with_no_valid_spectra() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    auto &spec = inputWS->getSpectrum(0);
    spec.clearDetectorIDs(); // clear the detectors to test the exception catching

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 11);
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), -1); // Spectrum No should be -1

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_With_Altered_Parameters() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    std::string outWSName{"Detector Table Test"};

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WorkspaceIndices", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludeData", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Check that a missing efixed value throws a runtime error
    const auto &spectrumInfo = inputWS->spectrumInfo();
    std::shared_ptr<const IDetector> detector(&spectrumInfo.detector(0), Mantid::NoDeleting());
    TS_ASSERT_THROWS(inputWS->getEFixed(detector), const std::runtime_error &);
    // Check that an invalid efixed value throws an invalid argument error
    auto &run = inputWS->mutableRun();
    run.addProperty("deltaE-mode", std::string("Direct"), true);
    run.addProperty("Ei", std::string("23423f42"));

    TS_ASSERT_THROWS(inputWS->getEFixed(detector), const std::invalid_argument &);

    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(outWSName));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 13);
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), 2); // Spectrum No should be 2 due to the WorkspaceIndex

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_Exec_Peaks_Workspace() {
    PeaksWorkspace_sptr inputWS = WorkspaceCreationHelper::createPeaksWorkspace(5, false);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 2);
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), 0); // First column is Index when exec on PeaksWorkspace, so expect 0

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Non_Peak_Table_Workspace_Throws_Exception() {
    ITableWorkspace_sptr inputWS = std::make_shared<TableWorkspace>();

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));

    TS_ASSERT_THROWS(alg.executeAsChildAlg(), const std::runtime_error &);
  }

  void test_Exec_Matrix_Workspace_with_Include_DetPos() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludeDetectorPosition", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 12); // extra column compared to test_Exec_Matrix_Workspace
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), 1); // Spectrum No should be 1, if not in the exception
    TS_ASSERT_EQUALS(ws->cell<V3D>(1, 11),
                     V3D(0.0, 0.0, -9.0)); // Last two are monitors, first position should be (0.0, 0.0, -9.0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_with_no_valid_spectra_include_DetPos() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    auto &spec = inputWS->getSpectrum(0);
    spec.clearDetectorIDs(); // clear the detectors to test the exception catching

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludeDetectorPosition", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 12);
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), -1);                  // Spectrum No should be -1
    TS_ASSERT_EQUALS(ws->cell<V3D>(0, 11), V3D(0.0, 0.0, 0.0)); // Detector Position should be (0.0, 0.0, 0.0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_with_Detector_ID_as_integers() {

    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
    std::string outWSName = "out_int_detid";

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OneRowPerDetectorID", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(outWSName));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->cell<int>(0, 2), 1);
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_with_Detector_ID_as_strings() {

    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
    std::string outWSName = "out_int_detid";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->cell<std::string>(0, 2), "1");
  }

  void test_index_column_is_int() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
    std::string outWSName = "out_int_detid";
    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws) {
      return;
    }
    const auto indexCol = ws->getColumn(0);
    TS_ASSERT_EQUALS("Index", indexCol->name());
    TS_ASSERT(indexCol->isType<int>());
  }

  void test_index_column_contains_sequential_workspace_indices() {
    // Without WorkspaceIndices set, the Index column should contain 0, 1, 2, ...
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10);
    std::string outWSName = "seq_index_test";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws) {
      return;
    }

    TS_ASSERT_EQUALS(ws->cell<int>(0, 0), 0);
    TS_ASSERT_EQUALS(ws->cell<int>(1, 0), 1);
    TS_ASSERT_EQUALS(ws->cell<int>(2, 0), 2);
  }

  void test_index_column_reflects_workspace_index_when_subset_selected() {
    // When WorkspaceIndices=[2], the single output row's Index should be 2 (the wsIndex), not 0 (the row number)
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10);
    std::string outWSName = "subset_index_test";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WorkspaceIndices", "2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws) {
      return;
    }

    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 0), 2); // Index stores the wsIndex (2), not the row number (0)
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), 3); // Spectrum No for wsIndex 2 is 3
  }

  void test_r_and_theta_are_valid_for_regular_detector() {
    // R (L2 distance) should be positive, theta should be in [0, 180] for a regular detector
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    std::string outWSName = "r_theta_test";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws) {
      return;
    }

    // Col 3 = R, Col 4 = Theta (no Q column since no efixed)
    const double R = ws->cell<double>(0, 3);
    const double theta = ws->cell<double>(0, 4);
    TS_ASSERT_LESS_THAN(0.0, R);
    TS_ASSERT_LESS_THAN_EQUALS(0.0, theta);
    TS_ASSERT_LESS_THAN_EQUALS(theta, 180.0);
    TS_ASSERT_EQUALS(ws->cell<std::string>(0, 6), "no"); // Monitor column
  }

  void test_monitor_row_has_correct_theta_and_positive_R() {
    // Monitors behind the sample (z < 0) should have theta=180 and positive R
    // create2DWorkspaceWithFullInstrument(3, 10, true) → rows 1 and 2 are monitors at (0,0,-9)
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
    std::string outWSName = "monitor_theta_test";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws) {
      return;
    }

    // Row 0: regular detector
    TS_ASSERT_EQUALS(ws->cell<std::string>(0, 6), "no");
    // Rows 1 and 2: monitors
    TS_ASSERT_EQUALS(ws->cell<std::string>(1, 6), "yes");
    TS_ASSERT_EQUALS(ws->cell<std::string>(2, 6), "yes");
    // Monitor R must be positive (absolute value of l2, which can be negative in DetectorInfo)
    TS_ASSERT_LESS_THAN(0.0, ws->cell<double>(1, 3));
    // Monitor behind the sample (position z=-9 < sampleDist=0) → theta=180
    TS_ASSERT_EQUALS(ws->cell<double>(1, 4), 180.0);
  }

  void test_populateTableByDetID_row_count_unconstrained_by_workspace_indices() {
    // When OneRowPerDetectorID=true the table iterates over all detector IDs,
    // so WorkspaceIndices is ignored and the row count equals the detector count.
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10);
    std::string outWSName = "pick_one_det_row_count";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WorkspaceIndices", "1"));     // only index 1 requested…
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OneRowPerDetectorID", true)); // …but this path ignores it
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws)
      return;

    // 3 detectors in the instrument → 3 rows, not 1
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
  }

  void test_populateTableByDetID_each_row_has_unique_detector_id() {
    // populateTableByDetID writes one distinct detector ID per row as an integer.
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10);
    std::string outWSName = "pick_one_det_unique_ids";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OneRowPerDetectorID", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Collect all detector IDs from col 2; every row must have a distinct integer ID.
    std::set<int> seenIds;
    for (size_t row = 0; row < ws->rowCount(); ++row)
      seenIds.insert(ws->cell<int>(row, 2));

    TS_ASSERT_EQUALS(seenIds.size(), ws->rowCount());
  }

  void test_populateTableByDetID_pads_missing_detector_rows_with_invalid_values() {
    // When a detector in detectorInfo is not mapped to any spectrum,
    // populateTableByDetID should write a placeholder row with wsIndex=-1,
    // specNo=-1, the correct detId, and isMonitor="n/a".
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    // Clear detector IDs from spectrum 0 so det ID 1 is present in
    // detectorInfo but not referenced by any spectrum.
    inputWS->getSpectrum(0).clearDetectorIDs();

    std::string outWSName = "det_id_padding_test";
    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OneRowPerDetectorID", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Row count equals detector count in the instrument, not the number of
    // spectra with valid detector mappings.
    TS_ASSERT_EQUALS(ws->rowCount(), 2);

    // Row 0: det ID 1, cleared from spectrum 0 → padded with invalid sentinel values.
    TS_ASSERT_EQUALS(ws->cell<int>(0, 0), -1);            // wsIndex sentinel
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), -1);            // specNo sentinel
    TS_ASSERT_EQUALS(ws->cell<int>(0, 2), 1);             // detId preserved from instrument
    TS_ASSERT_EQUALS(ws->cell<std::string>(0, 6), "n/a"); // isMonitor sentinel

    // Row 1: det ID 2, still mapped to spectrum 1 → valid data.
    TS_ASSERT_EQUALS(ws->cell<int>(1, 1), 2); // specNo=2
    TS_ASSERT_EQUALS(ws->cell<int>(1, 2), 2); // detId=2
  }

  void test_populateTableByDetID_monitor_physics_via_calculateWsIdxData() {
    // Exercises calculateWsIdxData through the populateTableByDetID code path:
    // monitor rows should carry positive R and theta=180.
    // create2DWorkspaceWithFullInstrument(3, 10, true) → 1 regular detector + 2 monitors.
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
    std::string outWSName = "pick_one_det_monitor_physics";

    CreateDetectorTable alg;
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OneRowPerDetectorID", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws = alg.getProperty("DetectorTableWorkspace");
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Rows 1 and 2 correspond to the monitors placed at z=-9 and z=-2 (behind sample).
    // Col 3 = R (must be positive), col 4 = theta (must be 180 for monitors behind sample),
    // col 6 = isMonitor string.
    TS_ASSERT_EQUALS(ws->cell<std::string>(0, 6), "no");  // row 0: regular detector
    TS_ASSERT_EQUALS(ws->cell<std::string>(1, 6), "yes"); // row 1: monitor
    TS_ASSERT_EQUALS(ws->cell<std::string>(2, 6), "yes"); // row 2: monitor
    TS_ASSERT_LESS_THAN(0.0, ws->cell<double>(1, 3));     // R must be positive
    TS_ASSERT_EQUALS(ws->cell<double>(1, 4), 180.0);      // theta=180 for monitor behind sample
  }
};

class CreateDetectorTablePerformance : public CxxTest::TestSuite {
public:
  MatrixWorkspace_sptr WS;
  CreateDetectorTable alg;

  void setUp() override { WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10000, 1000); }

  void tearDown() override { AnalysisDataService::Instance().remove(WS->getName() + "-Detectors"); }

  void testExec() {
    alg.initialize();
    alg.setProperty("InputWorkspace", WS);
    alg.execute();
  }
};
