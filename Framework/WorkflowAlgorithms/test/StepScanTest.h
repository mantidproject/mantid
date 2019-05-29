// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_ROCKINGCURVETEST_H_
#define MANTID_WORKFLOWALGORITHMS_ROCKINGCURVETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/FilterByXValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidWorkflowAlgorithms/StepScan.h"

using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::WorkflowAlgorithms::StepScan;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class StepScanTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StepScanTest *createSuite() { return new StepScanTest(); }
  static void destroySuite(StepScanTest *suite) { delete suite; }

  StepScanTest() : outWSName("outTable") {}

  void setUp() override {
    // I'm not sure why, but this trick seems to be needed to force linking to
    // the Algorithms
    // library on Ubuntu (otherwise it says the child algorithms are not
    // registered).
    Mantid::Algorithms::FilterByXValue dummy;
    dummy.version();
    // End of dummy code

    inputWS = WorkspaceCreationHelper::createEventWorkspace2(3, 1);
    inputWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    auto scan_index = new TimeSeriesProperty<int>("scan_index");
    scan_index->addValue("2010-01-01T00:00:00", 0);
    inputWS->mutableRun().addProperty(scan_index);
    auto prop = new TimeSeriesProperty<double>("sample_property");
    // This log goes from 1->5 half way through the scan_index=1 period (so
    // average will be 3)
    prop->addValue("2010-01-01T00:00:00", 1.0);
    prop->addValue("2010-01-01T00:01:05", 5.0);
    inputWS->mutableRun().addProperty(prop);

    stepScan = boost::make_shared<StepScan>();
    stepScan->initialize();
    stepScan->setProperty("InputWorkspace", inputWS);
    stepScan->setPropertyValue("OutputWorkspace", outWSName);
  }

  void test_the_basics() {
    TS_ASSERT_EQUALS(stepScan->name(), "StepScan");
    TS_ASSERT_EQUALS(stepScan->version(), 1);
    TS_ASSERT_EQUALS(stepScan->category(), "Workflow\\Alignment");
    TS_ASSERT(!stepScan->summary().empty());
  }

  void test_fail_on_invalid_inputs() {
    StepScan alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT(alg.execute());
  }

  // Just a simple test on a very small workspace - leave more extensive testing
  // for system tests
  void test_simple_case() {
    // Add a non-zero value to the scan_index log
    auto scan_index =
        inputWS->mutableRun().getTimeSeriesProperty<int>("scan_index");
    scan_index->addValue("2010-01-01T00:00:30", 1);
    // TODO: 'Close' the log, but I need to think about what happens if it isn't
    // closed
    scan_index->addValue("2010-01-01T00:01:40", 0);

    // Create a workspace to mask out one of the spectra
    MatrixWorkspace_sptr mask =
        WorkspaceFactory::Instance().create("MaskWorkspace", 3, 1, 1);
    mask->dataY(1)[0] = 1;

    TS_ASSERT_THROWS_NOTHING(stepScan->setProperty("MaskWorkspace", mask));
    TS_ASSERT_THROWS_NOTHING(stepScan->setProperty("XMin", 40.0));
    TS_ASSERT_THROWS_NOTHING(stepScan->setProperty("XMax", 90.0));
    TS_ASSERT(stepScan->execute());

    // Retrieve the output table workspace from the ADS.
    Mantid::API::ITableWorkspace_sptr table;
    TS_ASSERT_THROWS_NOTHING(
        table = AnalysisDataService::Instance()
                    .retrieveWS<Mantid::API::ITableWorkspace>(outWSName));
    TS_ASSERT(table);
    if (!table)
      return;

    TS_ASSERT_EQUALS(table->rowCount(), 1)
    TS_ASSERT_EQUALS(table->columnCount(), 6)
    TS_ASSERT_EQUALS(table->getColumnNames()[0], "scan_index");
    TS_ASSERT_EQUALS(table->Int(0, 0), 1)
    TS_ASSERT_EQUALS(table->getColumnNames()[1], "Counts");
    TS_ASSERT_EQUALS(table->getColumnNames()[2], "Error");
    // The original workspace has 600 events.
    // The scan_index=1 period covers 70 out of 100s -> so 420 events remain
    // The masking removes 1 of 3 spectra -> leaving 280
    // The XMin/XMax range covers 50s out of the remaining 70s TOF range
    //   (note that there's a correlation between pulse time & TOF) -> so 200
    //   are left at the end
    TS_ASSERT_EQUALS(table->Int(0, 1), 200)
    TS_ASSERT_EQUALS(table->Double(0, 2), std::sqrt(200.0))
    TS_ASSERT_EQUALS(table->getColumnNames()[3], "time");
    TS_ASSERT_EQUALS(table->Double(0, 3), 70.0);
    TS_ASSERT_EQUALS(table->getColumnNames()[4], "proton_charge");
    // The cell in the proton_charge column will be empty
    TS_ASSERT_EQUALS(table->getColumnNames()[5], "sample_property");
    TS_ASSERT_EQUALS(table->Double(0, 5), 3.0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_zero_row_not_removed_if_only_one() {
    TS_ASSERT(stepScan->execute());
    // Retrieve the output table workspace from the ADS.
    Mantid::API::ITableWorkspace_sptr table;
    TS_ASSERT_THROWS_NOTHING(
        table = AnalysisDataService::Instance()
                    .retrieveWS<Mantid::API::ITableWorkspace>(outWSName));

    TS_ASSERT_EQUALS(table->rowCount(), 1)
    TS_ASSERT_EQUALS(table->Int(0, 0), 0)
  }

private:
  EventWorkspace_sptr inputWS;
  IAlgorithm_sptr stepScan;
  const std::string outWSName;
};

#endif /* MANTID_WORKFLOWALGORITHMS_ROCKINGCURVETEST_H_ */
