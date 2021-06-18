// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/CombineDiffCal.h"
#include "MantidDataObjects/TableWorkspace.h"

using Mantid::Algorithms::CombineDiffCal;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::TableRow;
using Mantid::DataObjects::TableWorkspace;

class CombineDiffCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CombineDiffCalTest *createSuite() { return new CombineDiffCalTest(); }
  static void destroySuite(CombineDiffCalTest *suite) { delete suite; }

  MatrixWorkspace_sptr create_grouped_workspace() {
    // CreateSampleWorkspace(OutputWorkspace=name,
    // XUnit="dSpacing", NumBanks=1) gives 100 spectra with differenct locations
  }

  ITableWorkspace_sptr create_diffcal_table() {
    // create table with correct column names
    ITableWorkspace_sptr table = std::make_shared<TableWorkspace>();
    table->addColumn("int", "detid");
    table->addColumn("double", "difc");
    table->addColumn("double", "difa");
    table->addColumn("double", "tzero");

    // fill the values
    TableRow new_row = table->appendRow();
    //      new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;

    return table;
  }

  void test_init() {
    CombineDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // cases to cover (can be in the same dataset)
    // single pixel with pixel==group==arb
    // single pixel with pixel==arb!=group
    // single pixel with pixel==arb!=group
    // grouped with arb==group
    // grouped with arb!=group

    // test input
    auto difcal_table_pixel = create_diffcal_table();
    auto difcal_table_group = create_diffcal_table();
    // TODO create calibration data

    // set up algorithm
    CombineDiffCal alg;
    alg.setChild(true); // Don't put output in ADS by default
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelCalibration", difcal_table_pixel));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupedCalibration", difcal_table_group));
    // TODO  TS_ASSERT_THROWS_NOTHING( alg.setProperty("CalibrationWorkspace", difcal_table_pixel) );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));

    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    /*
    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_FAIL("TODO: Check the results and remove this line");
    */
  }
};
