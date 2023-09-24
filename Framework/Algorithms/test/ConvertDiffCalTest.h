// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <list>

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/ConvertDiffCal.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

using Mantid::Algorithms::ConvertDiffCal;
using namespace Mantid::API;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using Mantid::Kernel::V3D;

class ConvertDiffCalTest : public CxxTest::TestSuite {

public:
  enum COLUMNS : size_t { DETID = 0, DIFC = 1 };

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertDiffCalTest *createSuite() { return new ConvertDiffCalTest(); }
  static void destroySuite(ConvertDiffCalTest *suite) { delete suite; }

  void test_Init() {
    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /* specify fake entry in offset workspace or calibration table workspace */
  class fake_entry {
  public:
    enum {
      offset,      /* fake entry specifies an entry in the fake input offsets workspace */
      masked,      /* fake entry specifies masked offset workspace entry */
      unmasked,    /* fake entry specifies an unmasked offset workspace entry */
      calibration, /* fake entry specifies an entry in the input calibration table */
    };

    fake_entry(int detector_id, int workspace_type, double difc, int mask = fake_entry::unmasked, double difa = 0,
               double tzero = 0)
        : detector_id(detector_id), workspace_type(workspace_type), difc(difc), mask(mask), difa(difa), tzero(tzero) {}

    int detector_id;
    int workspace_type;
    double difc;
    int mask;
    double difa;
    double tzero;
  };

  /* contains a fake offset workspace and accompanying fake previous calibration workspace */
  class fake_workspaces {
  public:
    fake_workspaces(OffsetsWorkspace_sptr offsets, ITableWorkspace_sptr calibration_table)
        : offsets(offsets), calibration_table(calibration_table) {}

    OffsetsWorkspace_sptr offsets;

    ITableWorkspace_sptr calibration_table;
  };

  fake_workspaces generate_test_data(std::list<class fake_entry> const &entries) {
    Mantid::Geometry::Instrument_sptr instrument = ComponentCreationHelper::createEmptyInstrument();

    ITableWorkspace_sptr calibration_table = std::make_shared<Mantid::DataObjects::TableWorkspace>();

    calibration_table->addColumn("int", "detid");
    calibration_table->addColumn("double", "difc");
    calibration_table->addColumn("double", "difa");
    calibration_table->addColumn("double", "tzero");

    /* loop to add detectors and ids to instrument */
    for (auto const &entry : entries) {
      /* add an entry to the fake offset workspace by adding a detector there */
      if (entry.workspace_type == fake_entry::offset) {
        /* create a detector. Is this a memory leak? Idk. Depends what instrument does with it */
        Mantid::Geometry::Detector *det = new Mantid::Geometry::Detector("point-detector", entry.detector_id, nullptr);

        instrument->add(det);

        instrument->markAsDetector(det);
      }

      /* add an entry to the fake calibration workspace */
      else if (entry.workspace_type == fake_entry::calibration) {
        Mantid::API::TableRow new_row = calibration_table->appendRow();

        new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;
      }
    }

    /* create an offset workspace with the instrument */
    OffsetsWorkspace_sptr offsets = std::make_shared<OffsetsWorkspace>(instrument);
    Mantid::Geometry::DetectorInfo &d_info = offsets->mutableDetectorInfo();

    /* Loop to apply masks */
    for (auto const &entry : entries) {
      if (entry.workspace_type == fake_entry::offset) {
        size_t internal_index = d_info.indexOf(entry.detector_id);

        if (entry.mask == fake_entry::masked) {
          d_info.setMasked(internal_index, true);
        }

        else if (entry.mask == fake_entry::unmasked) {
          d_info.setMasked(internal_index, false);
        }

        offsets->setValue(entry.detector_id, entry.difc);
      }
    }

    return fake_workspaces(offsets, calibration_table);
  }

  void test_partial_update() {
    /* specify contents of fake workspaces */
    /* intentionally unsorted */
    std::list<class fake_entry> fake_entries = {
        /* 2 entries in the table that are not in the offset workspace - should be propagated */
        fake_entry(5, fake_entry::calibration, 5),
        fake_entry(6, fake_entry::calibration, 6),

        /* 2 entries in the offset workspace that are not in the table - should be updated */
        fake_entry(1, fake_entry::offset, 1.0, fake_entry::unmasked),
        fake_entry(0, fake_entry::offset, 1.0, fake_entry::unmasked),
        /* entries that are masked - should not be updated */
        fake_entry(3, fake_entry::offset, 3, fake_entry::masked),
        fake_entry(2, fake_entry::offset, 0, fake_entry::masked),
        /* 2 entries that exist in both. Existing values should be updated */
        fake_entry(7, fake_entry::offset, 7, fake_entry::unmasked),
        fake_entry(4, fake_entry::offset, 4, fake_entry::unmasked),

        /* 2 entries that exists in both - this one should be updated */
        fake_entry(4, fake_entry::calibration, 4),
        fake_entry(7, fake_entry::calibration, 7),
    };

    /* generate fake workspaces */
    class fake_workspaces fake_workspaces = generate_test_data(fake_entries);

    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", fake_workspaces.offsets));
    std::string updated_calibration_table_name("updated_calibration_table");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", updated_calibration_table_name));

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreviousCalibration", fake_workspaces.calibration_table));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws =
                                 AnalysisDataService::Instance().retrieveWS<Workspace>(updated_calibration_table_name));
    TS_ASSERT(ws);
    if (!ws)
      return;

    auto updated_calibration_table = std::dynamic_pointer_cast<ITableWorkspace>(ws);
    TS_ASSERT(updated_calibration_table);

    /* Get detector_ids */
    auto detector_id_column = updated_calibration_table->getColumn(0);
    /* check size: 4 existing entries from the offsets workspace and 2 new ones from the previous
     * calibration */
    int correct_size = 6;
    TS_ASSERT_EQUALS(detector_id_column->size(), correct_size);
    auto difc_column = updated_calibration_table->getColumn(1);
    TS_ASSERT_EQUALS(difc_column->size(), correct_size);

    TS_ASSERT_EQUALS(detector_id_column->toDouble(0), 0);
    TS_ASSERT_EQUALS(detector_id_column->toDouble(1), 1);
    TS_ASSERT_EQUALS(detector_id_column->toDouble(2), 4);
    TS_ASSERT_EQUALS(detector_id_column->toDouble(3), 5);
    TS_ASSERT_EQUALS(detector_id_column->toDouble(4), 6);
    TS_ASSERT_EQUALS(detector_id_column->toDouble(5), 7);

    /* check difc: */
    TS_ASSERT_EQUALS(difc_column->toDouble(0), 0);
    TS_ASSERT_EQUALS(difc_column->toDouble(1), 0);
    TS_ASSERT_EQUALS(difc_column->toDouble(2), 4.0 / 5.0); // detector id 4 should be updated
    TS_ASSERT_EQUALS(difc_column->toDouble(3), 5);         // detector id 5 should be propagated
    TS_ASSERT_EQUALS(difc_column->toDouble(4), 6);         // detector id 6 should be propagated
    TS_ASSERT_EQUALS(difc_column->toDouble(5), 7.0 / 8.0); // detector id 7 should be updated
  }

  void test_exec() {

    // Create a fake offsets workspace
    auto instr = ComponentCreationHelper::createMinimalInstrument(V3D(0., 0., -10.), // source
                                                                  V3D(0., 0., 0.),   // sample
                                                                  V3D(1., 0., 0.));  // detector

    OffsetsWorkspace_sptr offsets = std::make_shared<OffsetsWorkspace>(instr);

    /* Offsets of zero are ignored by default - before this convention, this unit test was using
     * an offset of zero. Changing it from zero to epsilon gets the test to pass */
    offsets->setValue(1, 0); // wksp_index=0, detid=1

    // Name of the output workspace.
    std::string outWSName("ConvertDiffCalTest_OutputWS");

    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", offsets));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // test various  values
    auto table = std::dynamic_pointer_cast<ITableWorkspace>(ws);
    TS_ASSERT(table);

    std::vector<std::string> columnNames = table->getColumnNames();
    TS_ASSERT_EQUALS(columnNames.size(), 4);
    TS_ASSERT_EQUALS(columnNames[0], "detid");
    TS_ASSERT_EQUALS(columnNames[1], "difc");

    auto detid = table->getColumn("detid");
    TS_ASSERT(detid);
    TS_ASSERT_EQUALS(detid->size(), 1);
    TS_ASSERT_EQUALS(detid->toDouble(0), 1.);

    auto difc = table->getColumn("difc");
    TS_ASSERT(difc);
    TS_ASSERT_DELTA(difc->toDouble(0), 3932.3, .1);

    auto difa = table->getColumn("difa");
    TS_ASSERT(difa);
    TS_ASSERT_EQUALS(difa->toDouble(0), 0.);

    auto tzero = table->getColumn("tzero");
    TS_ASSERT(tzero);
    TS_ASSERT_EQUALS(tzero->toDouble(0), 0.);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  // test that the algorithm will faill in absolute/relative mode using offsets <= -1
  void test_failure_negative_DIFC() {
    /**
     * In absolute/relative mode, the DIFC is updated as
     *   DIFC_new = DIFC_old / (1 + offset)
     * If offset <= -1, this produces negative DIFC_new, which lead to unphysical d-spacings
     * Make sure an error is thrown if this would happen.
     */

    ConvertDiffCal alg_useprev, alg_noprev;
    std::string new_calibration_table_name("updated_calibration_table");
    // test failure using previous calibration values
    TS_ASSERT_THROWS_NOTHING(alg_useprev.initialize());
    TS_ASSERT(alg_useprev.isInitialized());
    alg_useprev.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg_useprev.setPropertyValue("OutputWorkspace", new_calibration_table_name));
    TS_ASSERT_THROWS_NOTHING(alg_useprev.setProperty("OffsetMode", "Relative"));
    TS_ASSERT_THROWS_NOTHING(alg_useprev.setProperty("BinWidth", 1.0)); // will be unused

    // test failure if no previous calibration values
    TS_ASSERT_THROWS_NOTHING(alg_noprev.initialize());
    TS_ASSERT(alg_noprev.isInitialized());
    alg_noprev.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg_noprev.setPropertyValue("OutputWorkspace", new_calibration_table_name));
    TS_ASSERT_THROWS_NOTHING(alg_noprev.setProperty("OffsetMode", "Relative"));
    TS_ASSERT_THROWS_NOTHING(alg_noprev.setProperty("BinWidth", 1.0)); // will be unused

    // test failure if offset = -1.0
    std::list<class fake_entry> fake_entries_1{fake_entry(0, fake_entry::offset, -1.0),
                                               fake_entry(0, fake_entry::calibration, 2.0)};
    class fake_workspaces fake_workspaces_1 = generate_test_data(fake_entries_1);
    TS_ASSERT_THROWS_NOTHING(alg_useprev.setProperty("OffsetsWorkspace", fake_workspaces_1.offsets));
    TS_ASSERT_THROWS_NOTHING(alg_useprev.setProperty("PreviousCalibration", fake_workspaces_1.calibration_table));
    TS_ASSERT_THROWS(alg_useprev.execute(), const std::logic_error &);
    TS_ASSERT_THROWS_NOTHING(alg_noprev.setProperty("OffsetsWorkspace", fake_workspaces_1.offsets));
    TS_ASSERT_THROWS(alg_noprev.execute(), const std::logic_error &);

    // test failure if offset < -1.0
    std::list<class fake_entry> fake_entries_2{fake_entry(0, fake_entry::offset, -2.0),
                                               fake_entry(0, fake_entry::calibration, 2.0)};
    class fake_workspaces fake_workspaces_2 = generate_test_data(fake_entries_2);
    TS_ASSERT_THROWS_NOTHING(alg_useprev.setProperty("OffsetsWorkspace", fake_workspaces_2.offsets));
    TS_ASSERT_THROWS_NOTHING(alg_useprev.setProperty("PreviousCalibration", fake_workspaces_2.calibration_table));
    TS_ASSERT_THROWS(alg_useprev.execute(), const std::logic_error &);
    TS_ASSERT_THROWS_NOTHING(alg_noprev.setProperty("OffsetsWorkspace", fake_workspaces_2.offsets));
    TS_ASSERT_THROWS(alg_noprev.execute(), const std::logic_error &);
  }

  // test with 'OffsetMode' set to 'Signed'
  void test_signed_offset() {
    /**
     * With the offset and binwidth both set to 1, result should be to halve the original DIFC values
     *    DIFC_new = DIFC_old * (1+|DX|)^{-1} = DIFC_old * (2)^{-1} = DIFC_old/2
     * Setup a test dataset with powers of two as DIFC for easier verification, check all halved
     */
    const int LEN_TEST = 10;
    std::list<class fake_entry> fake_entries;
    std::array<double, LEN_TEST> expected_results;

    for (int i = 0; i < LEN_TEST; i++) {
      fake_entry offsetEntry(i, fake_entry::offset, 1.);
      fake_entry calEntry(i, fake_entry::calibration, std::pow(2, i));
      fake_entries.emplace_back(offsetEntry);
      fake_entries.emplace_back(calEntry);
      expected_results[i] = std::pow(2, i - 1);
    }

    /* generate fake workspaces */
    class fake_workspaces fake_workspaces = generate_test_data(fake_entries);

    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", fake_workspaces.offsets));
    std::string new_calibration_table_name("updated_calibration_table");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", new_calibration_table_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", 1.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreviousCalibration", fake_workspaces.calibration_table));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<Workspace>(new_calibration_table_name));
    TS_ASSERT(ws);
    if (!ws)
      return;

    auto updated_calibration_table = std::dynamic_pointer_cast<ITableWorkspace>(ws);
    TS_ASSERT(updated_calibration_table);

    /* Get detector_ids */
    auto detector_id_column = updated_calibration_table->getColumn(COLUMNS::DETID);
    int correct_size = 10;
    TS_ASSERT_EQUALS(detector_id_column->size(), correct_size);
    auto difc_column = updated_calibration_table->getColumn(COLUMNS::DIFC);
    TS_ASSERT_EQUALS(difc_column->size(), correct_size);

    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(detector_id_column->toDouble(i), i);
    }

    /* check difc: */
    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(difc_column->toDouble(i), expected_results[i])
    }
  }

  // test in 'Signed' offset mode with large negative offsets
  void test_signed_offset_large_negative() {
    /**
     * With the binwidth set to 1, offset = -2, result should quadruple original DIFC values
     *    DIFC_new = DIFC_old * (1+|DX|)^{-offset} = DIFC_old * (2)^{--2} = DIFC_old * 4.
     * Setup a test dataset with powers of two as DIFC for easier verification
     */
    const int LEN_TEST = 10;
    std::list<class fake_entry> fake_entries;
    std::array<double, LEN_TEST> expected_results;

    for (int i = 0; i < LEN_TEST; i++) {
      fake_entry offsetEntry(i, fake_entry::offset, -2.);
      fake_entry calEntry(i, fake_entry::calibration, std::pow(2, i));
      fake_entries.emplace_back(offsetEntry);
      fake_entries.emplace_back(calEntry);
      expected_results[i] = std::pow(2, i + 2);
    }

    /* generate fake workspaces */
    class fake_workspaces fake_workspaces = generate_test_data(fake_entries);

    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", fake_workspaces.offsets));
    std::string new_calibration_table_name("updated_calibration_table");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", new_calibration_table_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", 1.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreviousCalibration", fake_workspaces.calibration_table));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<Workspace>(new_calibration_table_name));
    TS_ASSERT(ws);
    if (!ws)
      return;

    auto updated_calibration_table = std::dynamic_pointer_cast<ITableWorkspace>(ws);
    TS_ASSERT(updated_calibration_table);

    /* Get detector_ids */
    auto detector_id_column = updated_calibration_table->getColumn(COLUMNS::DETID);
    int correct_size = 10;
    TS_ASSERT_EQUALS(detector_id_column->size(), correct_size);
    auto difc_column = updated_calibration_table->getColumn(COLUMNS::DIFC);
    TS_ASSERT_EQUALS(difc_column->size(), correct_size);

    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(detector_id_column->toDouble(i), i);
    }

    /* check difc: */
    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(difc_column->toDouble(i), expected_results[i])
    }
  }

  // test that zero offset does not change values
  void test_signed_zero_offset() {
    /**
     * Following formula, if the offset is zero, should be no change
     *   DIFC_new = DIFC_old * (1+|DX|)^{0} = DIFC_old * 1
     */
    std::list<class fake_entry> fake_entries;

    const int LEN_TEST = 10;
    for (int i = 0; i < LEN_TEST; i++) {
      fake_entry offsetEntry(i, fake_entry::offset, 0.);
      fake_entry calEntry(i, fake_entry::calibration, std::pow(2, i));
      fake_entries.emplace_back(offsetEntry);
      fake_entries.emplace_back(calEntry);
    }

    /* generate fake workspaces */
    class fake_workspaces fake_workspaces = generate_test_data(fake_entries);

    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", fake_workspaces.offsets));
    std::string updated_calibration_table_name("updated_calibration_table");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", updated_calibration_table_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", 1.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreviousCalibration", fake_workspaces.calibration_table));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    Workspace_sptr ws_old, ws_new;
    TS_ASSERT_THROWS_NOTHING(ws_new =
                                 AnalysisDataService::Instance().retrieveWS<Workspace>(updated_calibration_table_name));
    TS_ASSERT_THROWS_NOTHING(ws_old = fake_workspaces.calibration_table);
    TS_ASSERT(ws_old);
    TS_ASSERT(ws_new);
    if (!ws_new)
      return;

    auto updated_calibration_table = std::dynamic_pointer_cast<ITableWorkspace>(ws_new);
    auto original_calibration_table = std::dynamic_pointer_cast<ITableWorkspace>(ws_old);
    TS_ASSERT(updated_calibration_table);
    TS_ASSERT(original_calibration_table);

    /* Get detector_ids */
    auto detector_id_column_new = updated_calibration_table->getColumn(COLUMNS::DETID);
    auto detector_id_column_old = original_calibration_table->getColumn(COLUMNS::DETID);
    TS_ASSERT_EQUALS(detector_id_column_new->size(), LEN_TEST);
    TS_ASSERT_EQUALS(detector_id_column_old->size(), LEN_TEST);
    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(detector_id_column_new->toDouble(i), i);
      TS_ASSERT_EQUALS(detector_id_column_old->toDouble(i), i);
    }

    /* Get DIFC values, ensure equality new and old */
    auto difc_column_new = updated_calibration_table->getColumn(COLUMNS::DIFC);
    auto difc_column_old = original_calibration_table->getColumn(COLUMNS::DIFC);
    TS_ASSERT_EQUALS(difc_column_new->size(), LEN_TEST);
    TS_ASSERT_EQUALS(difc_column_old->size(), LEN_TEST);
    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(difc_column_new->toDouble(i), difc_column_old->toDouble(i));
    }
  }

  // test that zero binwidth does not change values
  void test_signed_zero_binwidth() {
    /**
     * Following formula, if the offset is zero, should be no change
     *   DIFC_new = DIFC_old * (1+|DX|)^{-offset} = DIFC_old * 1^{-offset} = DIFC_old
     */
    std::list<class fake_entry> fake_entries;

    const int LEN_TEST = 10;
    for (int i = 0; i < LEN_TEST; i++) {
      fake_entry offsetEntry(i, fake_entry::offset, 1.);
      fake_entry calEntry(i, fake_entry::calibration, std::pow(2, i));
      fake_entries.emplace_back(offsetEntry);
      fake_entries.emplace_back(calEntry);
    }

    /* generate fake workspaces */
    class fake_workspaces fake_workspaces = generate_test_data(fake_entries);

    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", fake_workspaces.offsets));
    std::string updated_calibration_table_name("updated_calibration_table");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", updated_calibration_table_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreviousCalibration", fake_workspaces.calibration_table));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    Workspace_sptr ws_old, ws_new;
    TS_ASSERT_THROWS_NOTHING(ws_new =
                                 AnalysisDataService::Instance().retrieveWS<Workspace>(updated_calibration_table_name));
    TS_ASSERT(ws_new);
    if (!ws_new)
      return;
    TS_ASSERT_THROWS_NOTHING(ws_old = fake_workspaces.calibration_table);
    //  AnalysisDataService::Instance().retrieveWS<Workspace>(fake_workspaces.calibration_table));
    TS_ASSERT(ws_old);

    auto updated_calibration_table = std::dynamic_pointer_cast<ITableWorkspace>(ws_new);
    TS_ASSERT(updated_calibration_table);
    auto original_calibration_table = std::dynamic_pointer_cast<ITableWorkspace>(ws_old);

    /* Get detector_ids */
    auto detector_id_column_new = updated_calibration_table->getColumn(COLUMNS::DETID);
    auto detector_id_column_old = original_calibration_table->getColumn(COLUMNS::DETID);

    TS_ASSERT_EQUALS(detector_id_column_new->size(), LEN_TEST);
    TS_ASSERT_EQUALS(detector_id_column_old->size(), LEN_TEST);
    auto difc_column_new = updated_calibration_table->getColumn(COLUMNS::DIFC);
    auto difc_column_old = original_calibration_table->getColumn(COLUMNS::DIFC);
    TS_ASSERT_EQUALS(difc_column_new->size(), LEN_TEST);
    TS_ASSERT_EQUALS(difc_column_old->size(), LEN_TEST);

    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(detector_id_column_new->toDouble(i), i);
      TS_ASSERT_EQUALS(detector_id_column_old->toDouble(i), i);
    }

    /* check that old and new difc are equal */
    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(difc_column_new->toDouble(i), difc_column_old->toDouble(i));
    }
  }

  // test that algorithm always uses absolute value of binwidth
  void test_abs_signed_bin() {
    /**
     * Signed mode is meant for logarithmic binning.
     * In logarithmic binning, it is required to specify binwidth as negative.
     * This can lead to sign confusions and erroneous calculations.
     * Ensure that the positive absolute value is always used by running with +/- and comparing.
     */
    const int LEN_TEST = 10;
    std::list<class fake_entry> fake_entries;
    for (int i = 0; i < LEN_TEST; i++) {
      fake_entry offsetEntry(i, fake_entry::offset, 0.5);
      fake_entry calEntry(i, fake_entry::calibration, std::pow(2, i));
      fake_entries.emplace_back(offsetEntry);
      fake_entries.emplace_back(calEntry);
    }

    /* generate fake workspaces */
    class fake_workspaces fake_workspaces = generate_test_data(fake_entries);

    Workspace_sptr ws1, ws2;
    // run once with a positive binwidth
    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreviousCalibration", fake_workspaces.calibration_table));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", fake_workspaces.offsets));
    std::string calibration_table_name_1("updated_calibration_table_once");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", calibration_table_name_1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", 0.5));
    // run once
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_THROWS_NOTHING(ws1 = AnalysisDataService::Instance().retrieveWS<Workspace>(calibration_table_name_1));

    // run again with a negative binwidth
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreviousCalibration", fake_workspaces.calibration_table));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", fake_workspaces.offsets));
    std::string calibration_table_name_2("updated_calibration_table_twice");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", calibration_table_name_2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", -0.5));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_THROWS_NOTHING(ws2 = AnalysisDataService::Instance().retrieveWS<Workspace>(calibration_table_name_2));

    // make sure we have the workspaces
    TS_ASSERT(ws1);
    TS_ASSERT(ws2);

    auto calibration_table_1 = std::dynamic_pointer_cast<ITableWorkspace>(ws1);
    auto calibration_table_2 = std::dynamic_pointer_cast<ITableWorkspace>(ws2);
    TS_ASSERT(calibration_table_1);
    TS_ASSERT(calibration_table_2);

    /* Get and compare difc values */
    auto difc_column_1 = calibration_table_1->getColumn(COLUMNS::DIFC);
    auto difc_column_2 = calibration_table_2->getColumn(COLUMNS::DIFC);
    TS_ASSERT_EQUALS(difc_column_1->size(), difc_column_2->size());

    /* check difc: */
    for (int i = 0; i < LEN_TEST; i++) {
      TS_ASSERT_EQUALS(difc_column_1->toDouble(i), difc_column_2->toDouble(i))
    }
  }

  void test_bad_offsetmode() {
    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("OffsetMode", "KAzoOooOBalOoO!"));
  }
};
