#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ProgressableViewMockObject.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::MantidWidgets;
using namespace testing;

class ReflDataProcessorPresenterTest : public CxxTest::TestSuite {

private:
  DataProcessorWhiteList createReflectometryWhiteList() {

    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace", "", true, "TOF_");
    whitelist.addElement("Angle", "ThetaIn", "");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "",
      true, "TRANS_");
    whitelist.addElement("Q min", "MomentumTransferMin", "");
    whitelist.addElement("Q max", "MomentumTransferMax", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    return whitelist;
  }

  std::map<std::string, DataProcessorPreprocessingAlgorithm>
    createReflectometryPreprocessMap() {

    return std::map<std::string, DataProcessorPreprocessingAlgorithm>{
      {"Run(s)", DataProcessorPreprocessingAlgorithm(
        "Plus", "TOF_",
        std::set<std::string>{"LHSWorkspace", "RHSWorkspace",
        "OutputWorkspace"})},
        { "Transmission Run(s)",
        DataProcessorPreprocessingAlgorithm(
          "CreateTransmissionWorkspaceAuto", "TRANS_",
          std::set<std::string>{"FirstTransmissionRun",
          "SecondTransmissionRun",
          "OutputWorkspace"}) }};
  }

  DataProcessorProcessingAlgorithm createReflectometryProcessor() {

    return DataProcessorProcessingAlgorithm(
      "ReflectometryReductionOneAuto",
      std::vector<std::string>{"IvsQ_binned_", "IvsQ_", "IvsLam_"},
      std::set<std::string>{"ThetaIn", "ThetaOut", "InputWorkspace",
      "OutputWorkspace", "OutputWorkspaceWavelength",
      "FirstTransmissionRun", "SecondTransmissionRun"});
  }

  DataProcessorPostprocessingAlgorithm createReflectometryPostprocessor() {

    return DataProcessorPostprocessingAlgorithm(
      "Stitch1DMany", "IvsQ_",
      std::set<std::string>{"InputWorkspaces", "OutputWorkspace"});
  }

  ITableWorkspace_sptr
    createWorkspace(const std::string &wsName,
      const DataProcessorWhiteList &whitelist) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    const int ncols = static_cast<int>(whitelist.size());

    auto colGroup = ws->addColumn("str", "Group");
    colGroup->setPlotType(0);

    for (int col = 0; col < ncols; col++) {
      auto column = ws->addColumn("str", whitelist.colNameFromColIndex(col));
      column->setPlotType(0);
    }

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr
    createPrefilledWorkspace(const std::string &wsName,
      const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
      << "12345"
      << "0.5"
      << ""
      << "0.1"
      << "1.6"
      << "0.04"
      << "1"
      << "ProcessingInstructions='0'";
    row = ws->appendRow();
    row << "0"
      << "12346"
      << "1.5"
      << ""
      << "1.4"
      << "2.9"
      << "0.04"
      << "1"
      << "ProcessingInstructions='0'";
    row = ws->appendRow();
    row << "1"
      << "24681"
      << "0.5"
      << ""
      << "0.1"
      << "1.6"
      << "0.04"
      << "1"

      << "";
    row = ws->appendRow();
    row << "1"
      << "24682"
      << "1.5"
      << ""
      << "1.4"
      << "2.9"
      << "0.04"
      << "1"

      << "";
    return ws;
  }

  void createTOFWorkspace(const std::string &wsName,
    const std::string &runNumber = "") {
    auto tinyWS =
      WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
    auto inst = tinyWS->getInstrument();

    inst->getParameterMap()->addDouble(inst.get(), "I0MonitorIndex", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "PointDetectorStart", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "PointDetectorStop", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "LambdaMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "LambdaMax", 10.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorBackgroundMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorBackgroundMax",
      10.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorIntegralMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorIntegralMax", 10.0);

    tinyWS->mutableRun().addLogData(
      new PropertyWithValue<double>("Theta", 0.12345));
    if (!runNumber.empty())
      tinyWS->mutableRun().addLogData(
        new PropertyWithValue<std::string>("run_number", runNumber));

    AnalysisDataService::Instance().addOrReplace(wsName, tinyWS);
  }


public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflDataProcessorPresenterTest *createSuite() {
    return new ReflDataProcessorPresenterTest();
  }
  static void destroySuite(ReflDataProcessorPresenterTest *suite) {
    delete suite;
  }

  ReflDataProcessorPresenterTest() { FrameworkManager::Instance(); }

  void testProcessNonEventWorkspaces() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    ReflDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params = \"0.1\""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspaces were created as expected
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12345"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12346"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12346");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12346");
    AnalysisDataService::Instance().remove("TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessEventWorkspaces() {}

  void testProcessEventWorkspacesCustomSlicing() {}

  void testPlotRow() {}

  void testPlotGroup() {}

  void testProcessWithNotebookWarn() {}

  void testProcessMixedEventAndNonEventWorkspacesWarn() {}

  void testPlotRowWarn() {}

  void testPlotGroupWarn() {}
};

#endif /* MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H */