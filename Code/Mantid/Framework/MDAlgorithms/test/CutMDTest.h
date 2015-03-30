#ifndef MANTID_MDALGORITHMS_CUTMDTEST_H_
#define MANTID_MDALGORITHMS_CUTMDTEST_H_

#include "MantidMDAlgorithms/CutMD.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
const std::string sharedWSName = "__CutMDTest_dataWS";
}

class CutMDTest : public CxxTest::TestSuite {
private:
  IMDWorkspace_sptr m_inWS;

public:
  CutMDTest() {
    FrameworkManager::Instance().exec("CreateMDWorkspace", 10,
        "OutputWorkspace", sharedWSName.c_str(),
        "Dimensions", "3",
        "Extents", "-10,10,-10,10,-10,10",
        "Names", "A,B,C",
        "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetSpecialCoordinates", 4,
        "InputWorkspace", sharedWSName.c_str(),
        "SpecialCoordinates", "HKL");

    FrameworkManager::Instance().exec("SetUB", 14,
        "Workspace", sharedWSName.c_str(),
        "a", "1",
        "b", "1",
        "c", "1",
        "alpha", "90",
        "beta", "90",
        "gamma", "90");

    FrameworkManager::Instance().exec("FakeMDEventData", 4,
        "InputWorkspace", sharedWSName.c_str(),
        "PeakParams", "10000,0,0,0,1");

    m_inWS =
      AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(sharedWSName);
  }

  virtual ~CutMDTest() { AnalysisDataService::Instance().remove(sharedWSName); }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CutMDTest *createSuite() { return new CutMDTest(); }
  static void destroySuite(CutMDTest *suite) { delete suite; }

  void test_init() {
    CutMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_throws_if_giving_4th_binning_param_when_workspace_is_3d() {
    const std::string wsName = "__CutMDTest_4thbinon3dthrows";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10,
        "OutputWorkspace", wsName.c_str(),
        "Dimensions", "3",
        "Extents", "-10,10,-10,10,-10,10",
        "Names", "H,K,L",
        "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetSpecialCoordinates", 4,
        "InputWorkspace", wsName.c_str(),
        "SpecialCoordinates", "HKL");

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", wsName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0.1");
    algCutMD->setProperty("P2Bin", "0.1");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("P4Bin", "0.1");
    TS_ASSERT_THROWS(algCutMD->execute(), std::runtime_error)

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_slice_to_original() {
    const std::string wsName = "__CutMDTest_slice_to_original";

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0.1");
    algCutMD->setProperty("P2Bin", "0.1");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("CheckAxes", false);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDEventWorkspace_sptr outWS = 
      AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_EQUALS(
        outWS->getDimension(0)->getMinimum(),
        m_inWS->getDimension(0)->getMinimum());
    TS_ASSERT_EQUALS(
        outWS->getDimension(0)->getMaximum(),
        m_inWS->getDimension(0)->getMaximum());
    TS_ASSERT_EQUALS(
        outWS->getDimension(1)->getMinimum(),
        m_inWS->getDimension(1)->getMinimum());
    TS_ASSERT_EQUALS(
        outWS->getDimension(1)->getMaximum(),
        m_inWS->getDimension(1)->getMaximum());
    TS_ASSERT_EQUALS(
        outWS->getDimension(2)->getMinimum(),
        m_inWS->getDimension(2)->getMinimum());
    TS_ASSERT_EQUALS(
        outWS->getDimension(2)->getMaximum(),
        m_inWS->getDimension(2)->getMaximum());

    TS_ASSERT_EQUALS("['zeta', 0, 0]", outWS->getDimension(0)->getName());
    TS_ASSERT_EQUALS("[0, 'eta', 0]", outWS->getDimension(1)->getName());
    TS_ASSERT_EQUALS("[0, 0, 'xi']", outWS->getDimension(2)->getName());

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_recalculate_extents_with_3_bin_arguments() {
    const std::string wsName = "__CutMDTest_recalc_extents_with_3_bin_args";

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0,0.3,0.8");
    algCutMD->setProperty("P2Bin", "0.1");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("CheckAxes", false);
    algCutMD->setProperty("NoPix", true);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDWorkspace_sptr outWS =
      AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_DELTA(outWS->getDimension(0)->getMinimum(), 0.0, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(0)->getMaximum(), 0.6, 1E-6);
    TS_ASSERT_EQUALS(outWS->getDimension(0)->getNBins(), 2);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_truncate_extents() {
    const std::string wsName = "__CutMDTest_truncate_extents";

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0,1.1,1");
    algCutMD->setProperty("P2Bin", "21");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("CheckAxes", false);
    algCutMD->setProperty("NoPix", true);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDWorkspace_sptr outWS =
      AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_EQUALS(outWS->getDimension(0)->getNBins(), 1);
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getNBins(), 1);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_wrong_proj_format_columns() {
    const std::string wsName = "__CutMDTest_wrong_proj_columns";

    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("Projection", proj);
    algCutMD->setProperty("P1Bin", "0.1");
    algCutMD->setProperty("P2Bin", "0.2");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("CheckAxes", false);
    TS_ASSERT_THROWS(algCutMD->execute(), std::runtime_error);
  }

  void test_wrong_proj_format_rows() {
    const std::string wsName = "__CutMDTest_wrong_proj_rows";

    // Correct columns
    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");
    proj->addColumn("str", "value");
    proj->addColumn("double", "offset");
    proj->addColumn("str", "type");
    // ...but no rows

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("Projection", proj);
    algCutMD->setProperty("P1Bin", "0.1");
    algCutMD->setProperty("P2Bin", "0.2");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("CheckAxes", false);
    TS_ASSERT_THROWS(algCutMD->execute(), std::runtime_error);
  }

  void test_orthogonal_slice_with_scaling() {
    const std::string wsName = "__CutMDTest_orthog_slice_with_scaling";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10,
        "OutputWorkspace", wsName.c_str(),
        "Dimensions", "3",
        "Extents", "-1,1,-1,1,-1,1",
        "Names", "H,K,L",
        "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetUB", 14,
        "Workspace", wsName.c_str(),
        "a", "1", "b", "1", "c", "1",
        "alpha", "90", "beta", "90", "gamma", "90");

    FrameworkManager::Instance().exec("SetSpecialCoordinates", 4,
        "InputWorkspace", wsName.c_str(),
        "SpecialCoordinates", "HKL");

    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");
    proj->addColumn("str", "value");
    proj->addColumn("double", "offset");
    proj->addColumn("str", "type");

    TableRow uRow = proj->appendRow();
    TableRow vRow = proj->appendRow();
    TableRow wRow = proj->appendRow();
    uRow << "u" << "1,0,0" << 0.0 << "r";
    vRow << "v" << "0,1,0" << 0.0 << "r";
    wRow << "w" << "0,0,1" << 0.0 << "r";

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", wsName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("Projection", proj);
    algCutMD->setProperty("P1Bin", "-0.5,0.5");
    algCutMD->setProperty("P2Bin", "-0.1,0.1");
    algCutMD->setProperty("P3Bin", "-0.3,0.3");
    algCutMD->setProperty("NoPix", true);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDHistoWorkspace_sptr outWS =
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_DELTA(outWS->getDimension(0)->getMinimum(), -0.5, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(0)->getMaximum(), 0.5, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(1)->getMinimum(), -0.1, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(1)->getMaximum(), 0.1, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(2)->getMinimum(), -0.3, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(2)->getMaximum(), 0.3, 1E-6);
    TS_ASSERT_EQUALS("['zeta', 0, 0]", outWS->getDimension(0)->getName());
    TS_ASSERT_EQUALS("[0, 'eta', 0]", outWS->getDimension(1)->getName());
    TS_ASSERT_EQUALS("[0, 0, 'xi']", outWS->getDimension(2)->getName());

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_non_orthogonal_slice() {
    const std::string wsName = "__CutMDTest_non_orthog_slice";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10,
        "OutputWorkspace", wsName.c_str(),
        "Dimensions", "3",
        "Extents", "-1,1,-1,1,-1,1",
        "Names", "H,K,L",
        "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetUB", 14,
        "Workspace", wsName.c_str(),
        "a", "1", "b", "1", "c", "1",
        "alpha", "90", "beta", "90", "gamma", "90");

    FrameworkManager::Instance().exec("SetSpecialCoordinates", 4,
        "InputWorkspace", wsName.c_str(),
        "SpecialCoordinates", "HKL");

    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");
    proj->addColumn("str", "value");
    proj->addColumn("double", "offset");
    proj->addColumn("str", "type");

    TableRow uRow = proj->appendRow();
    TableRow vRow = proj->appendRow();
    TableRow wRow = proj->appendRow();
    uRow << "u" << "1,1,0" << 0.0 << "r";
    vRow << "v" << "-1,1,0" << 0.0 << "r";
    wRow << "w" << "0,0,1" << 0.0 << "r";

    auto algCutMD = FrameworkManager::Instance().createAlgorithm("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", wsName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("Projection", proj);
    algCutMD->setProperty("P1Bin", "0.1");
    algCutMD->setProperty("P2Bin", "0.1");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("NoPix", true);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDHistoWorkspace_sptr outWS =
      AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_EQUALS(outWS->getDimension(0)->getMinimum(), -1);
    TS_ASSERT_EQUALS(outWS->getDimension(0)->getMaximum(), 1);
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getMinimum(), -1);
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getMaximum(), 1);
    TS_ASSERT_EQUALS(outWS->getDimension(2)->getMinimum(), -1);
    TS_ASSERT_EQUALS(outWS->getDimension(2)->getMaximum(), 1);
    TS_ASSERT_EQUALS("['zeta', 'zeta', 0]", outWS->getDimension(0)->getName());
    TS_ASSERT_EQUALS("['-eta', 'eta', 0]", outWS->getDimension(1)->getName());
    TS_ASSERT_EQUALS("[0, 0, 'xi']", outWS->getDimension(2)->getName());

    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /* MANTID_MDALGORITHMS_CUTMDTEST_H_ */
