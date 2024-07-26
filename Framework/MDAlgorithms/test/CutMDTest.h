// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/CutMD.h"

#include "../../API/inc/MantidAPI/IMDWorkspace.h"
#include "../../Kernel/inc/MantidKernel/MDUnit.h"
#include "../inc/MantidMDAlgorithms/CutMD.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/MDGeometry/QSample.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
const std::string sharedWSName = "__CutMDTest_dataWS";
const Mantid::API::MDNormalization eventNorm = Mantid::API::MDNormalization::VolumeNormalization;
const Mantid::API::MDNormalization histoNorm = Mantid::API::MDNormalization::NumEventsNormalization;
} // namespace

class CutMDTest : public CxxTest::TestSuite {
private:
  IMDWorkspace_sptr m_inWS;

  void addNormalization(const std::string &wsName) {
    auto ws = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName);
    auto eventWS = std::dynamic_pointer_cast<IMDEventWorkspace>(ws);
    auto histoWS = std::dynamic_pointer_cast<IMDHistoWorkspace>(ws);
    if (eventWS) {
      eventWS->setDisplayNormalization(eventNorm);
      eventWS->setDisplayNormalizationHisto(histoNorm);
    } else {
      histoWS->setDisplayNormalization(histoNorm);
    }
  }

  IMDWorkspace_const_sptr makeWorkspaceWithSpecifiedUnits(const std::string &units, const std::string &wsName) {
    const std::string units_string = units + "," + units + "," + units + ",V";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", wsName.c_str(), "Dimensions", "4",
                                      "Extents", "-1,1,-1,1,-1,1,-10,10", "Names", "H,K,L,E", "Units",
                                      units_string.c_str());

    FrameworkManager::Instance().exec("SetUB", 14, "Workspace", wsName.c_str(), "a", "1", "b", "1", "c", "1", "alpha",
                                      "90", "beta", "90", "gamma", "90");

    FrameworkManager::Instance().exec("FakeMDEventData", 4, "InputWorkspace", wsName.c_str(), "PeakParams",
                                      "1000,0,0,0,0,1");

    IMDWorkspace_sptr cutMDtestws = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName);

    auto eventWS = std::dynamic_pointer_cast<IMDEventWorkspace>(cutMDtestws);

    Mantid::Kernel::SpecialCoordinateSystem appliedCoord = Mantid::Kernel::QSample;
    eventWS->setCoordinateSystem(appliedCoord);

    FrameworkManager::Instance().exec("CreateSampleWorkspace", 4, "OutputWorkspace", "__CutMDTest_tempMatWS",
                                      "WorkspaceType", "Event");

    MatrixWorkspace_sptr tempMatWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__CutMDTest_tempMatWS");

    tempMatWS->mutableSample().setOrientedLattice(
        std::make_unique<Mantid::Geometry::OrientedLattice>(2, 3, 4, 90, 90, 90));

    ExperimentInfo_sptr ei(tempMatWS->cloneExperimentInfo());
    eventWS->addExperimentInfo(ei);

    AnalysisDataService::Instance().addOrReplace(wsName, eventWS);

    return eventWS;
  }

  ITableWorkspace_sptr createProjection(const std::string &units) {
    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");
    proj->addColumn("V3D", "value");
    proj->addColumn("double", "offset");
    proj->addColumn("str", "type");

    TableRow uRow = proj->appendRow();
    TableRow vRow = proj->appendRow();
    TableRow wRow = proj->appendRow();
    uRow << "u" << V3D(1, 0, 0) << 0.0 << units;
    vRow << "v" << V3D(0, 1, 0) << 0.0 << units;
    wRow << "w" << V3D(0, 0, 1) << 0.0 << units;

    return proj;
  }

  Mantid::Kernel::Logger m_log;

public:
  CutMDTest() : m_log("CutMDTest") {
    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", sharedWSName.c_str(), "Dimensions",
                                      "3", "Extents", "-10,10,-10,10,-10,10", "Names", "A,B,C", "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetUB", 14, "Workspace", sharedWSName.c_str(), "a", "1", "b", "1", "c", "1",
                                      "alpha", "90", "beta", "90", "gamma", "90");

    FrameworkManager::Instance().exec("FakeMDEventData", 4, "InputWorkspace", sharedWSName.c_str(), "PeakParams",
                                      "10000,0,0,0,1");

    m_inWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(sharedWSName);
    auto eventWS = std::dynamic_pointer_cast<IMDEventWorkspace>(m_inWS);
    eventWS->setDisplayNormalization(eventNorm);
    eventWS->setDisplayNormalizationHisto(histoNorm);
  }

  ~CutMDTest() override { AnalysisDataService::Instance().remove(sharedWSName); }

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

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", wsName.c_str(), "Dimensions", "3",
                                      "Extents", "-10,10,-10,10,-10,10", "Names", "H,K,L", "Units", "U,U,U");

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", wsName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0.1");
    algCutMD->setProperty("P2Bin", "0.1");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("P4Bin", "0.1");
    TS_ASSERT_THROWS(algCutMD->execute(), const std::runtime_error &)

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_slice_to_original() {
    const std::string wsName = "__CutMDTest_slice_to_original";

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0.1");
    algCutMD->setProperty("P2Bin", "0.1");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_EQUALS(outWS->getDimension(0)->getMinimum(), m_inWS->getDimension(0)->getMinimum());
    TS_ASSERT_EQUALS(outWS->getDimension(0)->getMaximum(), m_inWS->getDimension(0)->getMaximum());
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getMinimum(), m_inWS->getDimension(1)->getMinimum());
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getMaximum(), m_inWS->getDimension(1)->getMaximum());
    TS_ASSERT_EQUALS(outWS->getDimension(2)->getMinimum(), m_inWS->getDimension(2)->getMinimum());
    TS_ASSERT_EQUALS(outWS->getDimension(2)->getMaximum(), m_inWS->getDimension(2)->getMaximum());

    TS_ASSERT_EQUALS("['zeta', 0, 0]", outWS->getDimension(0)->getName());
    TS_ASSERT_EQUALS("[0, 'eta', 0]", outWS->getDimension(1)->getName());
    TS_ASSERT_EQUALS("[0, 0, 'xi']", outWS->getDimension(2)->getName());

    TSM_ASSERT_EQUALS("Should have num events normalization", outWS->displayNormalizationHisto(), histoNorm);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_recalculate_extents_with_3_bin_arguments() {
    const std::string wsName = "__CutMDTest_recalc_extents_with_3_bin_args";

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0,0.3,0.8");
    algCutMD->setProperty("P2Bin", "0.1");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("NoPix", true);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_DELTA(outWS->getDimension(0)->getMinimum(), 0.0, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(0)->getMaximum(), 0.6, 1E-6);
    TS_ASSERT_EQUALS(outWS->getDimension(0)->getNBins(), 2);

    TSM_ASSERT_EQUALS("Should have num events normalization", outWS->displayNormalizationHisto(), histoNorm);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_truncate_extents() {
    const std::string wsName = "__CutMDTest_truncate_extents";

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", sharedWSName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("P1Bin", "0,1.1,1");
    algCutMD->setProperty("P2Bin", "21");
    algCutMD->setProperty("P3Bin", "0.1");
    algCutMD->setProperty("NoPix", true);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_EQUALS(outWS->getDimension(0)->getNBins(), 1);
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getNBins(), 1);

    TSM_ASSERT_EQUALS("Should have num events normalization", outWS->displayNormalizationHisto(), histoNorm);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_orthogonal_slice_with_scaling() {
    const std::string wsName = "__CutMDTest_orthog_slice_with_scaling";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", wsName.c_str(), "Dimensions", "3",
                                      "Extents", "-1,1,-1,1,-1,1", "Names", "H,K,L", "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetUB", 14, "Workspace", wsName.c_str(), "a", "1", "b", "1", "c", "1", "alpha",
                                      "90", "beta", "90", "gamma", "90");

    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");
    proj->addColumn("V3D", "value");
    proj->addColumn("double", "offset");
    proj->addColumn("str", "type");

    TableRow uRow = proj->appendRow();
    TableRow vRow = proj->appendRow();
    TableRow wRow = proj->appendRow();
    uRow << "u" << V3D(1, 0, 0) << 0.0 << "r";
    vRow << "v" << V3D(0, 1, 0) << 0.0 << "r";
    wRow << "w" << V3D(0, 0, 1) << 0.0 << "r";

    addNormalization(wsName);

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
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

    IMDHistoWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(wsName);
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

    TSM_ASSERT_EQUALS("Should have num events normalization", outWS->displayNormalizationHisto(), histoNorm);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_non_orthogonal_slice() {
    const std::string wsName = "__CutMDTest_non_orthog_slice";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", wsName.c_str(), "Dimensions", "3",
                                      "Extents", "-1,1,-1,1,-1,1", "Names", "H,K,L", "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetUB", 14, "Workspace", wsName.c_str(), "a", "1", "b", "1", "c", "1", "alpha",
                                      "90", "beta", "90", "gamma", "90");

    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");
    proj->addColumn("V3D", "value");
    proj->addColumn("double", "offset");
    proj->addColumn("str", "type");

    TableRow uRow = proj->appendRow();
    TableRow vRow = proj->appendRow();
    TableRow wRow = proj->appendRow();
    uRow << "u" << V3D(1, 1, 0) << 0.0 << "r";
    vRow << "v" << V3D(-1, 1, 0) << 0.0 << "r";
    wRow << "w" << V3D(0, 0, 1) << 0.0 << "r";

    addNormalization(wsName);

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
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

    IMDHistoWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(wsName);
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

    TSM_ASSERT_EQUALS("Should have num events normalization", outWS->displayNormalizationHisto(), histoNorm);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_orthogonal_slice_with_cropping() {
    const std::string wsName = "__CutMDTest_orthog_slice_crop";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", wsName.c_str(), "Dimensions", "3",
                                      "Extents", "-1,1,-1,1,-1,1", "Names", "H,K,L", "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetUB", 14, "Workspace", wsName.c_str(), "a", "1", "b", "1", "c", "1", "alpha",
                                      "90", "beta", "90", "gamma", "90");

    auto proj = createProjection("r");

    addNormalization(wsName);

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
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

    IMDHistoWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(wsName);
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

    TSM_ASSERT_EQUALS("Should have num events normalization", outWS->displayNormalizationHisto(), histoNorm);
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_orthogonal_slice_4d() {
    const std::string wsName = "__CutMDTest_orthog_slice_4d";
    const std::string wsOutName = "__CutMDTest_orthog_slice_4d_out";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", wsName.c_str(), "Dimensions", "4",
                                      "Extents", "-1,1,-1,1,-1,1,-10,10", "Names", "H,K,L,E", "Units", "U,U,U,V");

    FrameworkManager::Instance().exec("SetUB", 14, "Workspace", wsName.c_str(), "a", "1", "b", "1", "c", "1", "alpha",
                                      "90", "beta", "90", "gamma", "90");

    addNormalization(wsName);

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", wsName);
    algCutMD->setProperty("OutputWorkspace", wsOutName);
    algCutMD->setProperty("P1Bin", "-0.5,0.5");
    algCutMD->setProperty("P2Bin", "-0.1,0.1");
    algCutMD->setProperty("P3Bin", "-0.3,0.3");
    algCutMD->setProperty("P4Bin", "1");
    algCutMD->setProperty("NoPix", true);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDHistoWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(wsOutName);
    TS_ASSERT(outWS.get());

    TS_ASSERT_DELTA(outWS->getDimension(0)->getMinimum(), -0.5, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(0)->getMaximum(), 0.5, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(1)->getMinimum(), -0.1, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(1)->getMaximum(), 0.1, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(2)->getMinimum(), -0.3, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(2)->getMaximum(), 0.3, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(3)->getMinimum(), -10, 1E-6);
    TS_ASSERT_DELTA(outWS->getDimension(3)->getMaximum(), 10, 1E-6);
    TS_ASSERT_EQUALS(20, outWS->getDimension(3)->getNBins());
    TS_ASSERT_EQUALS("['zeta', 0, 0]", outWS->getDimension(0)->getName());
    TS_ASSERT_EQUALS("[0, 'eta', 0]", outWS->getDimension(1)->getName());
    TS_ASSERT_EQUALS("[0, 0, 'xi']", outWS->getDimension(2)->getName());
    TS_ASSERT_EQUALS("E", outWS->getDimension(3)->getName());

    // Process again with a different binning
    algCutMD->setProperty("P4Bin", "-8,1,8");
    algCutMD->setProperty("InputWorkspace", wsName);
    algCutMD->setProperty("OutputWorkspace", wsOutName);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());
    outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(wsOutName);
    TS_ASSERT_EQUALS(16, outWS->getDimension(3)->getNBins());

    TSM_ASSERT_EQUALS("Should have num events normalization", outWS->displayNormalizationHisto(), histoNorm);
    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(wsOutName);
  }

  void test_MaxRecursionDepth_one() {
    // Test that with MaxRecursionDepth = 1, boxes are not split despite
    // many events
    const std::string wsName = "__CutMDTest_MaxRecursionDepth_one";

    FrameworkManager::Instance().exec("CreateMDWorkspace", 10, "OutputWorkspace", wsName.c_str(), "Dimensions", "3",
                                      "Extents", "-1,1,-1,1,-1,1", "Names", "H,K,L", "Units", "U,U,U");
    FrameworkManager::Instance().exec("FakeMDEventData", 6, "InputWorkspace", wsName.c_str(), "PeakParams",
                                      "2000,-0.5,-0.5,-0.5,0.1", "RandomizeSignal", "0");

    FrameworkManager::Instance().exec("SetUB", 14, "Workspace", wsName.c_str(), "a", "1", "b", "1", "c", "1", "alpha",
                                      "90", "beta", "90", "gamma", "90");

    FrameworkManager::Instance().exec("SetSpecialCoordinates", 4, "InputWorkspace", wsName.c_str(),
                                      "SpecialCoordinates", "HKL");

    ITableWorkspace_sptr proj = WorkspaceFactory::Instance().createTable();
    proj->addColumn("str", "name");
    proj->addColumn("V3D", "value");
    proj->addColumn("double", "offset");
    proj->addColumn("str", "type");

    TableRow uRow = proj->appendRow();
    TableRow vRow = proj->appendRow();
    TableRow wRow = proj->appendRow();
    uRow << "u" << V3D(1, 0, 0) << 0.0 << "r";
    vRow << "v" << V3D(0, 1, 0) << 0.0 << "r";
    wRow << "w" << V3D(0, 0, 1) << 0.0 << "r";

    addNormalization(wsName);

    auto algCutMD = AlgorithmManager::Instance().create("CutMD");
    algCutMD->initialize();
    algCutMD->setRethrows(true);
    algCutMD->setProperty("InputWorkspace", wsName);
    algCutMD->setProperty("OutputWorkspace", wsName);
    algCutMD->setProperty("Projection", proj);
    algCutMD->setProperty("P1Bin", "-1,1");
    algCutMD->setProperty("P2Bin", "-1,1");
    algCutMD->setProperty("P3Bin", "-1,1");
    algCutMD->setProperty("NoPix", false);
    algCutMD->execute();
    TS_ASSERT(algCutMD->isExecuted());

    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    TS_ASSERT(outWS.get());

    auto bc = outWS->getBoxController();
    TSM_ASSERT_EQUALS("Boxes should not have split into more than the 1 bin we "
                      "specificed in PnBin properties, because CutMD should "
                      "specify MaxRecursionDepth=1 in SliceMD",
                      bc->getTotalNumMDBoxes(), 1);

    // Clean up
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_slice_md_histo_workspace() {

    /*

    Input filled with 1's binning = 1 in each dimension
    ----------------------------- (10, 10)
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    |                           |
    -----------------------------
  (0, 0)


    Slice. Two vertical columns. Each 1 in width.

    ----------------------------- (10, 10)
    |                           |
    |                           |
    |__________________________ | (10, 7.1)
    |    |    |   ...           |
    |    |    |                 |
    |    |    |                 |
    |    |    |                 |
    |    |    |                 |
    |__________________________ | (10, 1.1)
    |                           |
    -----------------------------
  (0, 0)

  */

    using namespace Mantid::DataObjects;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal*/, 2 /*nd*/, 10 /*nbins*/,
                                                                            10 /*max*/, 1.0 /*error sq*/);

    CutMD alg; // This should be a pass-through to IntegrateMDHistoWorkspace
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    const double min = 1.1;
    const double max = 7.1; // 7.1 - 1.1 = 6
    const std::vector<double> minMaxVec = {min, max};

    alg.setProperty("P1Bin", std::vector<double>(0)); // Pass through. Do not change binning.
    alg.setProperty("P2Bin", minMaxVec);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Quick check that output seems to have the right shape.
    TSM_ASSERT_EQUALS("All integrated", 10,
                      outWS->getNPoints()); // one dimension unchanged the other integrated
    auto intdim = outWS->getDimension(1);
    TS_ASSERT_DELTA(min, intdim->getMinimum(), 1e-4);
    TS_ASSERT_DELTA(max, intdim->getMaximum(), 1e-4);
    TS_ASSERT_EQUALS(1, intdim->getNBins());
    auto dim = outWS->getDimension(0);
    TSM_ASSERT_DELTA("Not integrated binning should be the same as the original dimension", 0, dim->getMinimum(), 1e-4);
    TSM_ASSERT_DELTA("Not integrated binning should be the same as the original dimension", 10, dim->getMaximum(),
                     1e-4);
    TSM_ASSERT_EQUALS("Not integrated binning should be the same as the original dimension", 10, dim->getNBins());

    // Check the data.
    auto histoOutWS = std::dynamic_pointer_cast<IMDHistoWorkspace>(outWS);
    TS_ASSERT(histoOutWS);
    TSM_ASSERT_DELTA("Wrong integrated value", 6.0, histoOutWS->getSignalAt(0), 1e-4);
    TSM_ASSERT_DELTA("Wrong error value", std::sqrt(6.0 * (ws->getErrorAt(0) * ws->getErrorAt(0))),
                     histoOutWS->getErrorAt(0), 1e-4);
  }

  void test_findOriginalQUnits_invA() {
    const std::string ws_name = "__CutMDTest_unitstest";
    auto cutMDtestws = makeWorkspaceWithSpecifiedUnits("A^-1", ws_name);

    auto foundUnits = findOriginalQUnits(cutMDtestws, m_log);
    TSM_ASSERT_EQUALS("Units should be found to be inverse angstroms", foundUnits[0], "a");
    // Clean up
    AnalysisDataService::Instance().remove(ws_name);
  }

  void test_findOriginalQUnits_invAngstroms() {
    const std::string ws_name = "__CutMDTest_unitstest";
    auto cutMDtestws = makeWorkspaceWithSpecifiedUnits("Angstrom^-1", ws_name);

    auto foundUnits = findOriginalQUnits(cutMDtestws, m_log);
    TSM_ASSERT_EQUALS("Units should be found to be inverse angstroms", foundUnits[0], "a");
    // Clean up
    AnalysisDataService::Instance().remove(ws_name);
  }

  void test_findOriginalQUnits_rlu() {
    const std::string ws_name = "__CutMDTest_unitstest";
    // When units have been converted to RLU the unit label looks like this
    auto cutMDtestws = makeWorkspaceWithSpecifiedUnits("in 1.11A^-1", ws_name);

    auto foundUnits = findOriginalQUnits(cutMDtestws, m_log);
    TSM_ASSERT_EQUALS("Units should be found to be RLU", foundUnits[0], "r");
    // Clean up
    AnalysisDataService::Instance().remove(ws_name);
  }

  void test_CutMD_dimension_labels_A_to_A() {
    const std::string ws_name = "__CutMDTest_unitstest";
    auto ws_temp = makeWorkspaceWithSpecifiedUnits("Angstrom^-1", ws_name);
    TSM_ASSERT_EQUALS("Input workspace dimensions have units of inverse angstroms",
                      ws_temp->getDimension(0)->getUnits().ascii(), "Angstrom^-1");

    auto proj = createProjection("a");

    CutMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", ws_name);
    alg.setProperty("P1Bin", "-0.4,0.8");
    alg.setProperty("P2Bin", "-0.4,0.8");
    alg.setProperty("P3Bin", "-0.4,0.8");
    alg.setProperty("P4Bin", "-1.0,1.0");
    alg.setProperty("Projection", proj);
    alg.setProperty("NoPix", true);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outWS->getDimension(0)->getName(), "['zeta', 0, 0]")
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getName(), "[0, 'eta', 0]")
    TS_ASSERT_EQUALS(outWS->getDimension(2)->getName(), "[0, 0, 'xi']")
    TSM_ASSERT_EQUALS("Output workspace should have units of inverse angstroms",
                      outWS->getDimension(0)->getUnits().ascii(), "Angstrom^-1")

    // Clean up
    AnalysisDataService::Instance().remove(ws_name);
  }

  void test_CutMD_dimension_labels_A_to_RLU() {
    const std::string ws_name = "__CutMDTest_unitstest";
    auto ws_temp = makeWorkspaceWithSpecifiedUnits("Angstrom^-1", ws_name);
    TSM_ASSERT_EQUALS("Input workspace dimensions have units of inverse angstroms",
                      ws_temp->getDimension(0)->getUnits().ascii(), "Angstrom^-1");

    auto proj = createProjection("r");

    CutMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", ws_name);
    alg.setProperty("P1Bin", "-0.4,0.8");
    alg.setProperty("P2Bin", "-0.4,0.8");
    alg.setProperty("P3Bin", "-0.4,0.8");
    alg.setProperty("P4Bin", "-1.0,1.0");
    alg.setProperty("Projection", proj);
    alg.setProperty("NoPix", false);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outWS->getDimension(0)->getName(), "['zeta', 0, 0]")
    TS_ASSERT_EQUALS(outWS->getDimension(1)->getName(), "[0, 'eta', 0]")
    TS_ASSERT_EQUALS(outWS->getDimension(2)->getName(), "[0, 0, 'xi']")
    TSM_ASSERT_EQUALS("Output workspace unit label should show scaling from "
                      "conversion to RLU",
                      outWS->getDimension(0)->getUnits().ascii(), "in 3.14 A^-1")

    // Clean up
    AnalysisDataService::Instance().remove(ws_name);
  }
};
