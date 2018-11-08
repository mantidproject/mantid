// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SAMPLELOGSBEHAVIOURTEST_H_
#define MANTID_ALGORITHMS_SAMPLELOGSBEHAVIOURTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/make_cow.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::Algorithms::SampleLogsBehaviour;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

class SampleLogsBehaviourTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleLogsBehaviourTest *createSuite() {
    return new SampleLogsBehaviourTest();
  }
  static void destroySuite(SampleLogsBehaviourTest *suite) { delete suite; }

  // Please note that many tests are currently present in MergeRunsTest.

  void testMergeRunsIPFNames() {
    // Using default values of the constructor
    Logger log("testLog");
    MatrixWorkspace_sptr ws = create2DWorkspaceWithFullInstrument(
        3, 3, true, false, true, "INSTRUMENT");
    // Add sample logs
    ws->mutableRun().addLogData(new PropertyWithValue<double>("A", 2.65));
    ws->mutableRun().addLogData(new PropertyWithValue<double>("B", 1.56));
    ws->mutableRun().addLogData(new PropertyWithValue<double>("C", 8.55));
    // Load test parameter file
    LoadParameterFile paramLoader;
    paramLoader.initialize();
    paramLoader.setPropertyValue(
        "Filename", "IDFs_for_UNIT_TESTING/SampleLogsBehaviour_Parameters.xml");
    paramLoader.setProperty("Workspace", ws);
    paramLoader.execute();
    SampleLogsBehaviour sbh = SampleLogsBehaviour(ws, log);
    sbh.mergeSampleLogs(ws, ws);
    const std::string A = ws->run().getLogData("A")->value();
    const std::string B = ws->run().getLogData("B")->value();
    const std::string C = ws->run().getLogData("C")->value();
    TS_ASSERT_EQUALS(A, "2.6499999999999999")
    TS_ASSERT_EQUALS(B, "1.5600000000000001")
    TS_ASSERT_EQUALS(C, "8.5500000000000007")
  }

  void testConjoinXRunsIPFNames() {
    // Using suffix conjoin_ + default value names for constructing
    Logger log("testLog");
    MatrixWorkspace_sptr ws = create2DWorkspaceWithFullInstrument(
        3, 3, true, false, true, "INSTRUMENT");
    // Add sample logs
    ws->mutableRun().addLogData(new PropertyWithValue<double>("A", 2.65));
    ws->mutableRun().addLogData(new PropertyWithValue<double>("B", 1.56));
    ws->mutableRun().addLogData(new PropertyWithValue<double>("C", 8.55));
    // Load test parameter file
    LoadParameterFile paramLoader;
    paramLoader.initialize();
    paramLoader.setPropertyValue(
        "Filename", "IDFs_for_UNIT_TESTING/SampleLogsBehaviour_Parameters.xml");
    paramLoader.setProperty("Workspace", ws);
    paramLoader.execute();
    SampleLogsBehaviour sbh = SampleLogsBehaviour(
        ws, log, "", "", "", "", "", "", "", "conjoin_sample_logs_sum",
        "conjoin_sample_logs_time_series", "conjoin_sample_logs_list",
        "conjoin_sample_logs_warn", "conjoin_sample_logs_warn_tolerances",
        "conjoin_sample_logs_fail", "conjoin_sample_logs_fail_tolerances");
    sbh.mergeSampleLogs(ws, ws);
    const std::string A = ws->run().getLogData("A")->value();
    const std::string B = ws->run().getLogData("B")->value();
    const std::string C = ws->run().getLogData("C")->value();
    TS_ASSERT_EQUALS(A, "2.6499999999999999")
    TS_ASSERT_EQUALS(B, "1.5600000000000001")
    TS_ASSERT_EQUALS(C, "8.5500000000000007")
  }

private:
};

#endif /* MANTID_ALGORITHMS_SAMPLELOGSBEHAVIOURTEST_H_ */
