// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidMDAlgorithms/ConvToMDSelector.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/PreprocessDetectorsToMD.h"

#include "MantidAPI/AlgorithmManager.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

#include <utility>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

namespace {
Mantid::API::MatrixWorkspace_sptr createTestWorkspaces() {
  auto sim_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateSimulationWorkspace");
  sim_alg->initialize();
  sim_alg->setChild(true);
  sim_alg->setPropertyValue("Instrument", "MAR");
  sim_alg->setPropertyValue("BinParams", "-3,1,3");
  sim_alg->setPropertyValue("UnitX", "DeltaE");
  sim_alg->setPropertyValue("OutputWorkspace", "data_source_1");
  sim_alg->execute();

  Mantid::API::MatrixWorkspace_sptr ws = sim_alg->getProperty("OutputWorkspace");

  auto log_alg = Mantid::API::AlgorithmManager::Instance().create("AddSampleLog");
  log_alg->initialize();
  log_alg->setChild(true);
  log_alg->setProperty("Workspace", ws);
  log_alg->setPropertyValue("LogName", "Ei");
  log_alg->setPropertyValue("LogText", "3.0");
  log_alg->setPropertyValue("LogType", "Number");
  log_alg->execute();

  return ws;
}
} // namespace

class Convert2AnyTestHelper : public ConvertToMD {
public:
  Convert2AnyTestHelper() {};
  TableWorkspace_const_sptr preprocessDetectorsPositions(const Mantid::API::MatrixWorkspace_const_sptr &InWS2D,
                                                         const std::string &dEModeRequested = "Direct",
                                                         bool updateMasks = false) {
    return ConvertToMD::preprocessDetectorsPositions(InWS2D, dEModeRequested, updateMasks,
                                                     std::string(this->getProperty("PreprocDetectorsWS")));
  }
  void setSourceWS(Mantid::API::MatrixWorkspace_sptr InWS2D) { m_InWS2D = std::move(InWS2D); }
};
// helper function to provide list of names to test:
std::vector<std::string> dim_availible() { return {"DeltaE", "T", "alpha", "beta", "gamma"}; }
//
class ConvertToMDTest : public CxxTest::TestSuite {
  std::unique_ptr<Convert2AnyTestHelper> pAlg;

public:
  static ConvertToMDTest *createSuite() { return new ConvertToMDTest(); }
  static void destroySuite(ConvertToMDTest *suite) { delete suite; }

  using PropertyAllowedValues = std::vector<std::string>;

  void testInit() {

    TS_ASSERT_THROWS_NOTHING(pAlg->initialize())
    TS_ASSERT(pAlg->isInitialized())

    TSM_ASSERT_EQUALS("algorithm should have 26 properties", 26, (size_t)(pAlg->getProperties().size()));
  }

  void testSetUpThrow() {
    // TODO: check if wrong WS throws (should on validator)

    // get ws from the DS
    Mantid::API::MatrixWorkspace_sptr ws2D =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
    // give it to algorithm
    TSM_ASSERT_THROWS_NOTHING("the initial ws is not in the units of energy transfer",
                              pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    // target ws fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransferND"));
    // unknown Q-dimension trows
    TS_ASSERT_THROWS(pAlg->setPropertyValue("QDimensions", "unknownQ"), const std::invalid_argument &);
    // correct Q-dimension fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "|Q|"));
    // additional dimensions requested -- fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", "DeltaE,omega"));
  }

  void testExecNoQ() {

    Mantid::API::MatrixWorkspace_sptr ws2D =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
    auto pAxis = std::make_unique<API::NumericAxis>(3);
    pAxis->setUnit("dSpacing");

    ws2D->replaceAxis(0, std::move(pAxis));

    pAlg->setPropertyValue("InputWorkspace", "testWSProcessed");
    pAlg->setPropertyValue("OutputWorkspace", "WS3DNoQ");
    pAlg->setPropertyValue("PreprocDetectorsWS", "");
    pAlg->setPropertyValue("QDimensions", "CopyToMD");
    // Following 5 arguments should be ignored
    pAlg->setPropertyValue("Q3DFrames", "HKL");
    pAlg->setPropertyValue("QConversionScales", "HKL");
    pAlg->setPropertyValue("UProj", "0,0,1");
    pAlg->setPropertyValue("VProj", "1,0,0");
    pAlg->setPropertyValue("WProj", "0,1,0");

    pAlg->setPropertyValue("OtherDimensions", "phi,chi");
    //    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode",
    //    "NoDE"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Elastic")); // dE mode will be ignored
    //
    pAlg->setPropertyValue("MinValues", "-10,0,-10");
    pAlg->setPropertyValue("MaxValues", " 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    checkHistogramsHaveBeenStored("WS3DNoQ");

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("WS3DNoQ");
    TS_ASSERT_EQUALS(Mantid::Kernel::None, outWS->getSpecialCoordinateSystem());

    // Check that the display normalization is set correctly -- NoQ
    TSM_ASSERT_EQUALS("Should be set to volume normalization", outWS->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to volume normalization", outWS->displayNormalizationHisto(),
                      Mantid::API::VolumeNormalization);

    AnalysisDataService::Instance().remove("WS3DNoQ");
  }

  void testExecModQ() {

    Mantid::API::MatrixWorkspace_sptr ws2D =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
    auto pAxis = std::make_unique<API::NumericAxis>(3);
    pAxis->setUnit("dSpacing");

    ws2D->replaceAxis(0, std::move(pAxis));

    pAlg->setPropertyValue("OutputWorkspace", "WS3DmodQ");
    pAlg->setPropertyValue("InputWorkspace", "testWSProcessed");
    pAlg->setPropertyValue("QDimensions", "|Q|");
    pAlg->setPropertyValue("PreprocDetectorsWS", "");
    pAlg->setPropertyValue("OtherDimensions", "phi,chi");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Elastic"));
    //
    pAlg->setPropertyValue("MinValues", "-10,0,-10");
    pAlg->setPropertyValue("MaxValues", " 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    checkHistogramsHaveBeenStored("WS3DmodQ", 7000, 6489.5591101441796, 7300.7539989122024);

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("WS3DmodQ");
    TS_ASSERT_EQUALS(Mantid::Kernel::None, outWS->getSpecialCoordinateSystem());

    // Check that the display normalization is set correctly -- Q with
    // Elasticevent
    TSM_ASSERT_EQUALS("Should be set to volume normalization", outWS->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to volume normalization", outWS->displayNormalizationHisto(),
                      Mantid::API::VolumeNormalization);

    AnalysisDataService::Instance().remove("WS3DmodQ");
  }

  void testExecQ3D() {
    Mantid::API::MatrixWorkspace_sptr ws2D =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
    auto pAxis = std::make_unique<API::NumericAxis>(3);
    pAxis->setUnit("DeltaE");

    ws2D->replaceAxis(0, std::move(pAxis));

    pAlg->setPropertyValue("OutputWorkspace", "WS5DQ3D");
    pAlg->setPropertyValue("InputWorkspace", "testWSProcessed");
    pAlg->setPropertyValue("OtherDimensions", "phi,chi");
    pAlg->setPropertyValue("PreprocDetectorsWS", "");

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    pAlg->setPropertyValue("MinValues", "-10,-10,-10,  0,-10,-10");
    pAlg->setPropertyValue("MaxValues", " 10, 10, 10, 20, 40, 20");
    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Should finish successfully", pAlg->isExecuted());
    checkHistogramsHaveBeenStored("WS5DQ3D");

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("WS5DQ3D");
    TS_ASSERT_EQUALS(Mantid::Kernel::HKL, outWS->getSpecialCoordinateSystem());

    // Check that we are getting good frame information back for each dimension.
    TS_ASSERT_EQUALS("HKL", outWS->getDimension(0)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(0)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS("HKL", outWS->getDimension(1)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(1)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS("HKL", outWS->getDimension(2)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(2)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS(false, outWS->getDimension(3)->getMDUnits().isQUnit());

    // Check that the display normalization is set correctly -- WS2D with
    // inelastic and Q
    TSM_ASSERT_EQUALS("Should be set to num events normalization", outWS->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to num events normalization", outWS->displayNormalizationHisto(),
                      Mantid::API::NumEventsNormalization);
    AnalysisDataService::Instance().remove("WS5DQ3D");
  }

  void testInitialSplittingEnabled() {
    // Create workspace
    auto alg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("WorkspaceType", "Event");
    alg->setPropertyValue("OutputWorkspace", "dummy");
    alg->execute();

    Mantid::API::MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");

    Mantid::API::Run &run = ws->mutableRun();
    auto eiLog = new PropertyWithValue<double>("Ei", 12.0);
    run.addLogData(eiLog);

    ConvertToMD convertAlg;
    convertAlg.setChild(true);
    convertAlg.initialize();
    convertAlg.setPropertyValue("OutputWorkspace", "dummy");
    convertAlg.setProperty("InputWorkspace", ws);
    convertAlg.setProperty("QDimensions", "Q3D");
    convertAlg.setProperty("dEAnalysisMode", "Direct");
    convertAlg.setPropertyValue("MinValues", "-10,-10,-10, 0");
    convertAlg.setPropertyValue("MaxValues", " 10, 10, 10, 1");
    convertAlg.setPropertyValue("TopLevelSplitting", "1");
    convertAlg.execute();

    IMDEventWorkspace_sptr outEventWS = convertAlg.getProperty("OutputWorkspace");

    Mantid::API::BoxController_sptr boxController = outEventWS->getBoxController();
    std::vector<size_t> numMDBoxes = boxController->getNumMDBoxes();
    std::vector<size_t> numMDGridBoxes = boxController->getNumMDGridBoxes();
    // Check depth 0
    size_t level0 = numMDBoxes[0] + numMDGridBoxes[0];
    TSM_ASSERT_EQUALS("Should have no MDBoxes at level 0", 1, level0);
    // Check depth 1. The boxController is set to split with 50, 50, 50, 50.
    // We need to ensure that the number of Boxes plus the number of Gridboxes
    // is 50^4
    size_t level1 = numMDBoxes[1] + numMDGridBoxes[1];
    TSM_ASSERT_EQUALS("Should have 6250000 MDBoxes at level 1", 6250000, level1);

    // Confirm that the boxcontroller is set to the original settings
    TSM_ASSERT_EQUALS("Should be set to 5", 5, boxController->getSplitInto(0));
    TSM_ASSERT_EQUALS("Should be set to 5", 5, boxController->getSplitInto(1));
    TSM_ASSERT_EQUALS("Should be set to 5", 5, boxController->getSplitInto(2));
    TSM_ASSERT_EQUALS("Should be set to 5", 5, boxController->getSplitInto(3));

    // Check that the display normalization is set correctly -- EventWS with
    // inelastic and Q
    TSM_ASSERT_EQUALS("Should be set to num events normalization", outEventWS->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to num events normalization", outEventWS->displayNormalizationHisto(),
                      Mantid::API::NoNormalization);
  }

  void testInitialSplittingDisabled() {
    Mantid::API::MatrixWorkspace_sptr ws2D =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
    auto pAxis = std::make_unique<API::NumericAxis>(3);
    pAxis->setUnit("DeltaE");

    ws2D->replaceAxis(0, std::move(pAxis));

    pAlg->setPropertyValue("OutputWorkspace", "WS5DQ3D");
    pAlg->setPropertyValue("InputWorkspace", "testWSProcessed");
    pAlg->setPropertyValue("OtherDimensions", "phi,chi");
    pAlg->setPropertyValue("PreprocDetectorsWS", "");

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    pAlg->setPropertyValue("MinValues", "-10,-10,-10,  0,-10,-10");
    pAlg->setPropertyValue("MaxValues", " 10, 10, 10, 20, 40, 20");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("TopLevelSplitting", "0"));
    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Should finish successfully", pAlg->isExecuted());

    IMDEventWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("WS5DQ3D");
    Mantid::API::BoxController_sptr boxController = outputWS->getBoxController();

    std::vector<size_t> numMDBoxes = boxController->getNumMDBoxes();

    // Check depth 0
    TSM_ASSERT_EQUALS("Should have no MDBoxes at level 0", 0, numMDBoxes[0]);
    // Check depth 1. The boxController is set to split with 5, 5, 5, 5, 5, 5
    TSM_ASSERT_EQUALS("Should have 15625 MDBoxes at level 1", 15625, numMDBoxes[1]);

    // Confirm that the boxcontroller is set to the original settings
    TSM_ASSERT_EQUALS("Should be set to 5", 5, boxController->getSplitInto(0));
    TSM_ASSERT_EQUALS("Should be set to 5", 5, boxController->getSplitInto(1));
    TSM_ASSERT_EQUALS("Should be set to 5", 5, boxController->getSplitInto(2));

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("WS5DQ3D");
    TS_ASSERT_EQUALS(Mantid::Kernel::HKL, outWS->getSpecialCoordinateSystem());

    AnalysisDataService::Instance().remove("WS5DQ3D");
  }

  // DO NOT DISABLE THIS TEST
  void testAlgorithmProperties() {
    /*
    The Create MD Workspace GUI runs this algorithm internally.
    If property names and property allowed values here change, that interface
    will break.

    This unit test is designed to flag up changes here. If property values and
    names here do need to be changed,
    1) They must also be updated in CreateMDWorkspaceAlgDialog.cpp.
    2) It should then be confirmed that that the Create MD Workspace custom
    interface still works!
    3) Finally this unit test should be updated so that the tests pass.
    */

    ConvertToMD alg;
    alg.initialize();

    Mantid::Kernel::Property *QDimProperty;
    TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken "
                              "Create MD Workspace GUI. Fix "
                              "CreateMDWorkspaceGUI!",
                              QDimProperty = alg.getProperty("QDimensions"));
    TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken "
                              "Create MD Workspace GUI. Fix "
                              "CreateMDWorkspaceGUI!",
                              QDimProperty = alg.getProperty("dEAnalysisMode"));
    TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. "
                              "Fix CreateMDWorkspaceGUI!",
                              QDimProperty = alg.getProperty("OtherDimensions"));
    TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken "
                              "Create MD Workspace GUI. Fix "
                              "CreateMDWorkspaceGUI!",
                              QDimProperty = alg.getProperty("MinValues"));
    TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken "
                              "Create MD Workspace GUI. Fix "
                              "CreateMDWorkspaceGUI!",
                              QDimProperty = alg.getProperty("MaxValues"));

    QDimProperty = alg.getProperty("QDimensions");
    PropertyAllowedValues QDimValues = QDimProperty->allowedValues();
    TSM_ASSERT_EQUALS("QDimensions property values have changed. This has "
                      "broken Create MD Workspace GUI. Fix "
                      "CreateMDWorkspaceGUI!",
                      3, QDimValues.size());
    TSM_ASSERT("QDimensions property values have changed. This has broken "
               "Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",
               findValue(QDimValues, "CopyToMD"));
    TSM_ASSERT("QDimensions property values have changed. This has broken "
               "Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",
               findValue(QDimValues, "|Q|"));
    TSM_ASSERT("QDimensions property values have changed. This has broken "
               "Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",
               findValue(QDimValues, "Q3D"));

    Mantid::Kernel::Property *dEAnalysisMode = alg.getProperty("dEAnalysisMode");
    PropertyAllowedValues dEAnalysisModeValues = dEAnalysisMode->allowedValues();
    TSM_ASSERT_EQUALS("QDimensions property values have changed. This has "
                      "broken Create MD Workspace GUI. Fix "
                      "CreateMDWorkspaceGUI!",
                      3, dEAnalysisModeValues.size());
    //  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken
    //  Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",  findValue(
    //  dEAnalysisModeValues, "NoDE") );
    TSM_ASSERT("dEAnalysisMode property values have changed. This has broken "
               "Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",
               findValue(dEAnalysisModeValues, "Direct"));
    TSM_ASSERT("dEAnalysisMode property values have changed. This has broken "
               "Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",
               findValue(dEAnalysisModeValues, "Indirect"));
    TSM_ASSERT("dEAnalysisMode property values have changed. This has broken "
               "Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",
               findValue(dEAnalysisModeValues, "Elastic"));
  }

  ConvertToMDTest() {
    pAlg = std::make_unique<Convert2AnyTestHelper>();
    Mantid::API::MatrixWorkspace_sptr ws2D =
        WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4, 10, true);
    // rotate the crystal by twenty degrees back;
    ws2D->mutableRun().mutableGoniometer().setRotationAngle(0, 20);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 13., "meV", true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
  }
  ~ConvertToMDTest() override { AnalysisDataService::Instance().remove("testWSProcessed"); }

  void test_execute_filebackend() {
    // Arrange
    std::string file_name = "convert_to_md_test_file.nxs";
    if (Poco::File(file_name).exists())
      Poco::File(file_name).remove();
    {
      auto test_workspace = createTestWorkspaces();
      Algorithm_sptr min_max_alg = AlgorithmManager::Instance().createUnmanaged("ConvertToMDMinMaxGlobal");
      min_max_alg->initialize();
      min_max_alg->setChild(true);
      min_max_alg->setProperty("InputWorkspace", test_workspace);
      min_max_alg->setProperty("QDimensions", "Q3D");
      min_max_alg->setProperty("dEAnalysisMode", "Direct");
      min_max_alg->executeAsChildAlg();
      std::string min_values = min_max_alg->getPropertyValue("MinValues");
      std::string max_values = min_max_alg->getPropertyValue("MaxValues");

      Algorithm_sptr convert_alg = AlgorithmManager::Instance().createUnmanaged("ConvertToMD");
      convert_alg->initialize();
      convert_alg->setChild(true);
      convert_alg->setProperty("InputWorkspace", test_workspace);
      convert_alg->setProperty("QDimensions", "Q3D");
      convert_alg->setProperty("QConversionScales", "HKL");
      convert_alg->setProperty("dEAnalysisMode", "Direct");
      convert_alg->setPropertyValue("MinValues", min_values);
      convert_alg->setPropertyValue("MaxValues", max_values);

      // Act
      convert_alg->setProperty("Filename", file_name);
      convert_alg->setProperty("FileBackEnd", true);
      convert_alg->setProperty("OutputWorkspace", "blank");
      TS_ASSERT_THROWS_NOTHING(convert_alg->execute());
      Mantid::API::IMDEventWorkspace_sptr out_ws = convert_alg->getProperty("OutputWorkspace");

      // Asssert
      file_name = out_ws->getBoxController()->getFilename();

      auto load_alg = AlgorithmManager::Instance().createUnmanaged("LoadMD");
      load_alg->initialize();
      load_alg->setChild(true);
      load_alg->setProperty("Filename", file_name);
      load_alg->setProperty("FileBackEnd", true);
      load_alg->setProperty("OutputWorkspace", "blank");
      TS_ASSERT_THROWS_NOTHING(load_alg->execute());
      Mantid::API::IMDWorkspace_sptr reference_out_ws = load_alg->getProperty("OutputWorkspace");
      // Make sure that the output workspaces exist
      TS_ASSERT(out_ws);
      TS_ASSERT(reference_out_ws);
      auto ws_cast = std::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace_sptr>(reference_out_ws);

      // Compare the loaded and original workspace
      auto compare_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CompareMDWorkspaces");
      compare_alg->setChild(true);
      compare_alg->initialize();
      compare_alg->setProperty("Workspace1", out_ws);
      compare_alg->setProperty("Workspace2", reference_out_ws);
      compare_alg->setProperty("Tolerance", 0.00001);
      compare_alg->setProperty("CheckEvents", true);
      compare_alg->setProperty("IgnoreBoxID", true);
      TS_ASSERT_THROWS_NOTHING(compare_alg->execute());
      bool is_equal = compare_alg->getProperty("Equals");
      TS_ASSERT(is_equal);
    }

    // Remove the file
    if (Poco::File(file_name).exists()) {
      Poco::File(file_name).remove();
    }
  }

private:
  void checkHistogramsHaveBeenStored(const std::string &wsName, double val = 0.34, double bin_min = 0.3,
                                     double bin_max = 0.4) {
    IMDEventWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    uint16_t nexpts = outputWS->getNumExperimentInfo();
    for (uint16_t i = 0; i < nexpts; ++i) {
      ExperimentInfo_const_sptr expt = outputWS->getExperimentInfo(i);
      std::pair<double, double> bin = expt->run().histogramBinBoundaries(val);
      TS_ASSERT_DELTA(bin.first, bin_min, 1e-8);
      TS_ASSERT_DELTA(bin.second, bin_max, 1e-8);
    }
  }

  bool findValue(const PropertyAllowedValues &container, const std::string &value) {
    return std::find(container.begin(), container.end(), value) != container.end();
  }
};

//-------------------------------------------------------------------------------------------------
// Performance Test
//-------------------------------------------------------------------------------------------------

class ConvertToMDTestPerformance : public CxxTest::TestSuite {
  // Kernel::CPUTimer Clock;
  time_t start, end;

  size_t numHist;
  Kernel::Matrix<double> Rot;

  Mantid::API::MatrixWorkspace_sptr inWs2D;
  Mantid::API::MatrixWorkspace_sptr inWsEv;

  Mantid::MDAlgorithms::ConvertToMD convertAlgDefault;
  Mantid::MDAlgorithms::ConvertToMD convertAlgIndexed;

  WorkspaceCreationHelper::StubAlgorithm reporter;

  std::shared_ptr<ConvToMDBase> pConvMethods;
  DataObjects::TableWorkspace_sptr pDetLoc_events;
  DataObjects::TableWorkspace_sptr pDetLoc_histo;
  // pointer to mock algorithm to work with progress bar
  std::unique_ptr<WorkspaceCreationHelper::StubAlgorithm> pMockAlgorithm;

  std::shared_ptr<MDEventWSWrapper> pTargWS;

public:
  static ConvertToMDTestPerformance *createSuite() { return new ConvertToMDTestPerformance(); }
  static void destroySuite(ConvertToMDTestPerformance *suite) { delete suite; }

  void test_EventNoUnitsConv() {

    auto pAxis0 = std::make_unique<API::NumericAxis>(2);
    pAxis0->setUnit("DeltaE");
    inWsEv->replaceAxis(0, std::move(pAxis0));

    MDWSDescription WSD;
    std::vector<double> min(4, -1e+30), max(4, 1e+30);
    WSD.setMinMax(min, max);

    WSD.buildFromMatrixWS(inWsEv, "Q3D", "Indirect");

    WSD.m_PreprDetTable = pDetLoc_events;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("EXP_INFO_INDEX", static_cast<uint16_t>(10), true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();
    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWsEv, pConvMethods);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->initialize(WSD, pTargWS, false));

    pMockAlgorithm->resetProgress(numHist);
    // Clock.elapsedCPU();
    std::time(&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time(&end);
    double sec = std::difftime(end, start);
    TS_WARN("Time to complete: <EventWSType,Q3D,Indir,ConvertNo,CrystType>: " + boost::lexical_cast<std::string>(sec) +
            " sec");
  }

  void test_EventFromTOFConv() {

    auto pAxis0 = std::make_unique<API::NumericAxis>(2);
    pAxis0->setUnit("TOF");
    inWsEv->replaceAxis(0, std::move(pAxis0));

    MDWSDescription WSD;
    std::vector<double> min(4, -1e+30), max(4, 1e+30);
    WSD.setMinMax(min, max);
    WSD.buildFromMatrixWS(inWsEv, "Q3D", "Indirect");

    WSD.m_PreprDetTable = pDetLoc_events;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("EXP_INFO_INDEX", static_cast<uint16_t>(10), true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();
    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWsEv, pConvMethods);
    pConvMethods->initialize(WSD, pTargWS, false);

    pMockAlgorithm->resetProgress(numHist);
    // Clock.elapsedCPU();
    std::time(&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time(&end);
    double sec = std::difftime(end, start);
    // float sec = Clock.elapsedCPU();
    TS_WARN("Time to complete: <EventWSType,Q3D,Indir,ConvFromTOF,CrystType>: " +
            boost::lexical_cast<std::string>(sec) + " sec");
  }

  void test_HistoFromTOFConv() {

    auto pAxis0 = std::make_unique<API::NumericAxis>(2);
    pAxis0->setUnit("TOF");
    inWs2D->replaceAxis(0, std::move(pAxis0));

    MDWSDescription WSD;
    std::vector<double> min(4, -1e+30), max(4, 1e+30);
    WSD.setMinMax(min, max);

    WSD.buildFromMatrixWS(inWs2D, "Q3D", "Indirect");

    WSD.m_PreprDetTable = pDetLoc_histo;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("EXP_INFO_INDEX", static_cast<uint16_t>(10), true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();
    pTargWS->createEmptyMDWS(WSD);

    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWs2D, pConvMethods);
    pConvMethods->initialize(WSD, pTargWS, false);

    pMockAlgorithm->resetProgress(numHist);
    // Clock.elapsedCPU();
    std::time(&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time(&end);
    double sec = std::difftime(end, start);

    TS_WARN("Time to complete: <Ws2DHistoType,Q3D,Indir,ConvFromTOF,CrystType>: " +
            boost::lexical_cast<std::string>(sec) + " sec");
  }

  void test_HistoNoUnitsConv() {

    auto pAxis0 = std::make_unique<API::NumericAxis>(2);
    pAxis0->setUnit("DeltaE");
    inWs2D->replaceAxis(0, std::move(pAxis0));

    MDWSDescription WSD;
    std::vector<double> min(4, -1e+30), max(4, 1e+30);
    WSD.setMinMax(min, max);

    WSD.buildFromMatrixWS(inWs2D, "Q3D", "Indirect");

    WSD.m_PreprDetTable = pDetLoc_histo;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("EXP_INFO_INDEX", static_cast<uint16_t>(10), true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();
    pTargWS->createEmptyMDWS(WSD);

    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWs2D, pConvMethods);
    pConvMethods->initialize(WSD, pTargWS, false);

    pMockAlgorithm->resetProgress(numHist);
    // Clock.elapsedCPU();
    std::time(&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time(&end);
    double sec = std::difftime(end, start);

    TS_WARN("Time to complete: <Ws2DHistoType,Q3D,Indir,ConvertNo,CrystType>: " +
            boost::lexical_cast<std::string>(sec) + " sec");
  }

  void test_EventFromTOFConvBuildTreeDefault() { convertAlgDefault.execute(); }

  void test_EventFromTOFConvBuildTreeIndexed() { convertAlgIndexed.execute(); }

  static void setUpConvAlg(Mantid::MDAlgorithms::ConvertToMD &convAlg, const std::string &type,
                           const std::string &inName) {
    static uint32_t cnt = 0;
    std::vector<int> splits(3, 2);
    convAlg.initialize();
    convAlg.setProperty("SplitInto", splits);
    convAlg.setProperty("SplitThreshold", 10);
    convAlg.setPropertyValue("InputWorkspace", inName);
    convAlg.setPropertyValue("OutputWorkspace", std::to_string(cnt++) + "_ws");
    convAlg.setProperty("QDimensions", "Q3D");
    convAlg.setProperty("dEAnalysisMode", "Elastic");
    convAlg.setProperty("Q3DFrames", "Q_lab");
    convAlg.setProperty("ConverterType", type);
    convAlg.setRethrows(true);
  }

  ConvertToMDTestPerformance() : Rot(3, 3) {
    numHist = 100 * 100;
    size_t nEvents = 1000;
    inWsEv = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::createRandomEventWorkspace(nEvents, numHist, 0.1));
    inWsEv->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(int(numHist)));
    inWsEv->mutableRun().addProperty("Ei", 12., "meV", true);
    API::AnalysisDataService::Instance().addOrReplace("TestEventWS", inWsEv);

    inWs2D = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(int(numHist), int(nEvents)));
    // add workspace energy
    inWs2D->mutableRun().addProperty("Ei", 12., "meV", true);
    API::AnalysisDataService::Instance().addOrReplace("TestMatrixWS", inWs2D);

    auto pAlg = std::make_unique<PreprocessDetectorsToMD>();
    pAlg->initialize();

    pAlg->setPropertyValue("InputWorkspace", "TestMatrixWS");
    pAlg->setPropertyValue("OutputWorkspace", "PreprocessedDetectorsTable");

    pAlg->execute();
    if (!pAlg->isExecuted())
      throw(std::runtime_error("Can not preprocess histogram detectors to MD"));

    API::Workspace_sptr tWs = API::AnalysisDataService::Instance().retrieve("PreprocessedDetectorsTable");
    pDetLoc_histo = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(tWs);
    if (!pDetLoc_histo)
      throw(std::runtime_error("Can not obtain preprocessed histogram detectors "));

    pAlg->setPropertyValue("InputWorkspace", "TestEventWS");
    pAlg->execute();
    if (!pAlg->isExecuted())
      throw(std::runtime_error("Can not preprocess events detectors to MD"));

    tWs = API::AnalysisDataService::Instance().retrieve("PreprocessedDetectorsTable");
    pDetLoc_events = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(tWs);
    if (!pDetLoc_events)
      throw(std::runtime_error("Can not obtain preprocessed events detectors "));

    pTargWS = std::make_shared<MDEventWSWrapper>();

    Rot.setRandom(100);
    Rot.toRotation();

    // this will be used to display progress
    pMockAlgorithm = std::make_unique<WorkspaceCreationHelper::StubAlgorithm>();

    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("WorkspaceType", "Event");
    alg->setProperty("Function", "Flat background");
    alg->setProperty("XMin", 10000.0);
    alg->setProperty("XMax", 100000.0);
    alg->setProperty("NumEvents", 1000);
    alg->setProperty("BankPixelWidth", 20);
    alg->setProperty("Random", false);
    std::string inWsSampleName = "dummy";
    alg->setPropertyValue("OutputWorkspace", inWsSampleName);
    alg->setRethrows(true);
    alg->execute();

    setUpConvAlg(convertAlgDefault, "Default", inWsSampleName);
    setUpConvAlg(convertAlgIndexed, "Indexed", inWsSampleName);
  }
};
