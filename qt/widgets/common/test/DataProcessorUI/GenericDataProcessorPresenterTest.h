// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERTEST_H
#define MANTID_MANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERTEST_H
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/MockObjects.h"
#include "MantidQtWidgets/Common/MockProgressableView.h"
#include "MantidQtWidgets/Common/WidgetDllOption.h"
#include "MantidTestHelpers/DataProcessorTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace DataProcessorTestHelper;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
auto const DEFAULT_GROUP_NUMBER = 0;

// Use this if you need the Test class to be a friend of the data processor
// presenter
class GenericDataProcessorPresenterFriend
    : public GenericDataProcessorPresenter {
  friend class GenericDataProcessorPresenterTest;

public:
  // Standard constructor
  GenericDataProcessorPresenterFriend(
      const WhiteList &whitelist,
      const std::map<QString, PreprocessingAlgorithm> &preprocessingStep,
      const ProcessingAlgorithm &processor,
      const PostprocessingAlgorithm &postprocessor, int group,
      const std::map<QString, QString> &postprocessMap =
          std::map<QString, QString>(),
      const QString &loader = "Load")
      : GenericDataProcessorPresenter(whitelist, std::move(preprocessingStep),
                                      processor, postprocessor, group,
                                      postprocessMap, loader) {}

  // Delegating constructor (no pre-processing required)
  GenericDataProcessorPresenterFriend(
      const WhiteList &whitelist, const ProcessingAlgorithm &processor,
      const PostprocessingAlgorithm &postprocessor, int group)
      : GenericDataProcessorPresenter(whitelist, processor, postprocessor,
                                      group) {}

  // Delegating constructor (no pre- or post-processing required)
  GenericDataProcessorPresenterFriend(const WhiteList &whitelist,
                                      const ProcessingAlgorithm &processor,
                                      int group)
      : GenericDataProcessorPresenter(whitelist, processor, group) {}

  // Destructor
  ~GenericDataProcessorPresenterFriend() override {}
};

// Use this mocked presenter for tests that will start the reducing row/group
// workers/threads. This overrides the async methods to be non-async, allowing
// them to be tested.
class GenericDataProcessorPresenterNoThread
    : public GenericDataProcessorPresenter {
public:
  // Standard constructor
  GenericDataProcessorPresenterNoThread(
      const WhiteList &whitelist,
      const std::map<QString, PreprocessingAlgorithm> &preprocessingStep,
      const ProcessingAlgorithm &processor,
      const PostprocessingAlgorithm &postprocessor, int group,
      const std::map<QString, QString> &postprocessMap =
          std::map<QString, QString>(),
      const QString &loader = "Load")
      : GenericDataProcessorPresenter(whitelist, std::move(preprocessingStep),
                                      processor, postprocessor, group,
                                      postprocessMap, loader) {}

  // Delegating constructor (no pre-processing required)
  GenericDataProcessorPresenterNoThread(
      const WhiteList &whitelist, const ProcessingAlgorithm &processor,
      const PostprocessingAlgorithm &postprocessor, int group)
      : GenericDataProcessorPresenter(whitelist, processor, postprocessor,
                                      group) {}

  // Destructor
  ~GenericDataProcessorPresenterNoThread() override {}

private:
  // non-async row reduce
  void startAsyncRowReduceThread(RowData_sptr rowData, const int rowIndex,
                                 const int groupIndex) override {
    try {
      reduceRow(rowData);
      m_manager->update(groupIndex, rowIndex, rowData->data());
      m_manager->setProcessed(true, rowIndex, groupIndex);
    } catch (std::exception &ex) {
      reductionError(QString(ex.what()));
      rowThreadFinished(1);
    }
    rowThreadFinished(0);
  }

  // non-async group reduce
  void startAsyncGroupReduceThread(GroupData &groupData,
                                   int groupIndex) override {
    try {
      postProcessGroup(groupData);
      if (m_manager->rowCount(groupIndex) == static_cast<int>(groupData.size()))
        m_manager->setProcessed(true, groupIndex);
    } catch (std::exception &ex) {
      reductionError(QString(ex.what()));
      groupThreadFinished(1);
    }
    groupThreadFinished(0);
  }

  // Overriden non-async methods have same implementation as parent class
  void process(TreeData itemsToProcess) override {
    GenericDataProcessorPresenter::process(itemsToProcess);
  }
  void plotRow() override { GenericDataProcessorPresenter::plotRow(); }
  void plotGroup() override { GenericDataProcessorPresenter::plotGroup(); }
};

class GenericDataProcessorPresenterTest : public CxxTest::TestSuite {

private:
  using RowList = std::map<int, std::set<int>>;
  using GroupList = std::set<int>;

  WhiteList createReflectometryWhiteList() {

    WhiteList whitelist;
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

  const std::map<QString, PreprocessingAlgorithm>
  createReflectometryPreprocessingStep() {
    return {{"Run(s)", PreprocessingAlgorithm(
                           "Plus", "TOF_", "+",
                           std::set<QString>{"LHSWorkspace", "RHSWorkspace",
                                             "OutputWorkspace"})},
            {"Transmission Run(s)",
             PreprocessingAlgorithm("CreateTransmissionWorkspaceAuto", "TRANS_",
                                    "_",
                                    std::set<QString>{"FirstTransmissionRun",
                                                      "SecondTransmissionRun",
                                                      "OutputWorkspace"})}};
  }

  ProcessingAlgorithm createReflectometryProcessor() {
    return ProcessingAlgorithm(
        "ReflectometryReductionOneAuto",
        std::vector<QString>{"IvsQ_binned_", "IvsQ_", "IvsLam_"}, 1,
        std::set<QString>{"ThetaIn", "ThetaOut", "InputWorkspace",
                          "OutputWorkspace", "OutputWorkspaceWavelength",
                          "FirstTransmissionRun", "SecondTransmissionRun"});
  }

  PostprocessingAlgorithm createReflectometryPostprocessor() {

    return PostprocessingAlgorithm(
        "Stitch1DMany", "IvsQ_",
        std::set<QString>{"InputWorkspaces", "OutputWorkspace"});
  }

  ITableWorkspace_sptr createWorkspace(const QString &wsName,
                                       const WhiteList &whitelist) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    const int ncols = static_cast<int>(whitelist.size());

    auto colGroup = ws->addColumn("str", "Group");
    colGroup->setPlotType(0);

    for (int col = 0; col < ncols; col++) {
      auto column = ws->addColumn("str", whitelist.name(col).toStdString());
      column->setPlotType(0);
    }

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName.toStdString(), ws);

    return ws;
  }

  void createTOFWorkspace(const QString &wsName,
                          const QString &runNumber = "") {
    auto tinyWS =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            2000);
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
    if (!runNumber.isEmpty())
      tinyWS->mutableRun().addLogData(new PropertyWithValue<std::string>(
          "run_number", runNumber.toStdString()));

    AnalysisDataService::Instance().addOrReplace(wsName.toStdString(), tinyWS);
  }

  void createMultiPeriodTOFWorkspace(const QString &wsName,
                                     const QString &runNumber = "") {

    createTOFWorkspace(wsName + "_1", runNumber);
    createTOFWorkspace(wsName + "_2", runNumber);

    auto stdWorkspaceName = wsName.toStdString();

    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(
        AnalysisDataService::Instance().retrieve(stdWorkspaceName + "_1"));
    group->addWorkspace(
        AnalysisDataService::Instance().retrieve(stdWorkspaceName + "_2"));

    AnalysisDataService::Instance().addOrReplace(stdWorkspaceName, group);
  }

  ITableWorkspace_sptr createPrefilledWorkspace(const QString &wsName,
                                                const WhiteList &whitelist) {
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
        << "ProcessingInstructions='1'";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << ""
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"
        << "ProcessingInstructions='1'";
    row = ws->appendRow();
    row << "1"
        << "24681"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "ProcessingInstructions='1'";
    row = ws->appendRow();
    row << "1"
        << "24682"
        << "1.5"
        << ""
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"
        << "ProcessingInstructions='1'";

    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledWorkspaceThreeGroups(const QString &wsName,
                                      const WhiteList &whitelist) {
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
        << "";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << ""
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"
        << "";
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
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "2"
        << "30000"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "2"
        << "30001"
        << "1.5"
        << ""
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledWorkspaceWithTrans(const QString &wsName,
                                    const WhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
        << "12345"
        << "0.5"
        << "11115"
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"

        << "";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << "11116"
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"

        << "";
    row = ws->appendRow();
    row << "1"
        << "24681"
        << "0.5"
        << "22221"
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"

        << "";
    row = ws->appendRow();
    row << "1"
        << "24682"
        << "1.5"
        << "22222"
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"

        << "";
    return ws;
  }

  std::unique_ptr<GenericDataProcessorPresenterFriend> makeDefaultPresenter() {
    return std::make_unique<GenericDataProcessorPresenterFriend>(
        createReflectometryWhiteList(), createReflectometryPreprocessingStep(),
        createReflectometryProcessor(), createReflectometryPostprocessor(),
        DEFAULT_GROUP_NUMBER);
  }

  std::unique_ptr<GenericDataProcessorPresenterNoThread>
  makeDefaultPresenterNoThread() {
    return std::make_unique<GenericDataProcessorPresenterNoThread>(
        createReflectometryWhiteList(), createReflectometryPreprocessingStep(),
        createReflectometryProcessor(), createReflectometryPostprocessor(),
        DEFAULT_GROUP_NUMBER);
  }

  // Expect the view's widgets to be set in a particular state according to
  // whether processing or not
  void expectUpdateViewState(MockDataProcessorView &mockDataProcessorView,
                             Cardinality numTimes, bool isProcessing) {
    // Update menu items according to whether processing or not
    EXPECT_CALL(mockDataProcessorView, updateMenuEnabledState(isProcessing))
        .Times(numTimes);

    // These widgets are only enabled if not processing
    EXPECT_CALL(mockDataProcessorView, setProcessButtonEnabled(!isProcessing))
        .Times(numTimes);
    EXPECT_CALL(mockDataProcessorView, setInstrumentComboEnabled(!isProcessing))
        .Times(numTimes);
    EXPECT_CALL(mockDataProcessorView, setTreeEnabled(!isProcessing))
        .Times(numTimes);
    EXPECT_CALL(mockDataProcessorView, setOutputNotebookEnabled(!isProcessing))
        .Times(numTimes);
  }

  // Expect the view's widgets to be set in the paused state
  void
  expectUpdateViewToPausedState(MockDataProcessorView &mockDataProcessorView,
                                Cardinality numTimes) {
    expectUpdateViewState(mockDataProcessorView, numTimes, false);
  }

  // Expect the view's widgets to be set in the processing state
  void expectUpdateViewToProcessingState(
      MockDataProcessorView &mockDataProcessorView, Cardinality numTimes) {
    expectUpdateViewState(mockDataProcessorView, numTimes, true);
  }

  void expectGetSelection(MockDataProcessorView &mockDataProcessorView,
                          Cardinality numTimes, RowList rowlist = RowList(),
                          GroupList grouplist = GroupList()) {

    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(numTimes);
      EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(numTimes);
    } else {
      EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
          .Times(numTimes)
          .WillRepeatedly(Return(rowlist));
      EXPECT_CALL(mockDataProcessorView, getSelectedParents())
          .Times(numTimes)
          .WillRepeatedly(Return(grouplist));
    }
  }

  void expectGetOptions(MockMainPresenter &mockMainPresenter,
                        Cardinality numTimes,
                        std::string postprocessingOptions = "") {
    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockMainPresenter, getPreprocessingOptions()).Times(numTimes);
      EXPECT_CALL(mockMainPresenter, getProcessingOptions()).Times(numTimes);
      EXPECT_CALL(mockMainPresenter, getPostprocessingOptionsAsString())
          .Times(numTimes);
    } else {
      EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
          .Times(numTimes)
          .WillRepeatedly(Return(ColumnOptionsQMap()));
      EXPECT_CALL(mockMainPresenter, getProcessingOptions())
          .Times(numTimes)
          .WillRepeatedly(Return(OptionsQMap()));
      EXPECT_CALL(mockMainPresenter, getPostprocessingOptionsAsString())
          .Times(numTimes)
          .WillRepeatedly(
              Return(QString::fromStdString(postprocessingOptions)));
    }
  }

  void expectNotebookIsDisabled(MockDataProcessorView &mockDataProcessorView,
                                Cardinality numTimes) {
    // Call to check whether the notebook is enabled
    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockDataProcessorView, getEnableNotebook()).Times(numTimes);
    } else {
      EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
          .Times(numTimes)
          .WillRepeatedly(Return(false));
    }

    // Result is false, so never request the path
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(Exactly(0));
  }

  void expectNotebookIsEnabled(MockDataProcessorView &mockDataProcessorView,
                               Cardinality numTimes) {
    // Call to check whether the notebook is enabled
    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockDataProcessorView, getEnableNotebook()).Times(numTimes);
    } else {
      EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
          .Times(numTimes)
          .WillRepeatedly(Return(true));
    }

    // Result is false, so never request the path
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(numTimes);
  }

  void expectGetWorkspace(MockDataProcessorView &mockDataProcessorView,
                          Cardinality numTimes, const char *workspaceName) {
    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen()).Times(numTimes);
    } else {
      EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
          .Times(numTimes)
          .WillRepeatedly(Return(workspaceName));
    }
  }

  void expectAskUserWorkspaceName(MockDataProcessorView &mockDataProcessorView,
                                  Cardinality numTimes,
                                  const char *workspaceName = "") {
    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockDataProcessorView,
                  askUserString(_, _, QString("Workspace")))
          .Times(numTimes);
    } else {
      EXPECT_CALL(mockDataProcessorView,
                  askUserString(_, _, QString("Workspace")))
          .Times(numTimes)
          .WillOnce(Return(workspaceName));
    }
  }

  void expectAskUserYesNo(MockDataProcessorView &mockDataProcessorView,
                          Cardinality numTimes, const bool answer = false) {

    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockDataProcessorView, askUserYesNo(_, _)).Times(numTimes);
    } else {
      EXPECT_CALL(mockDataProcessorView, askUserYesNo(_, _))
          .Times(numTimes)
          .WillOnce(Return(answer));
    }
  }

  void expectNoWarningsOrErrors(MockDataProcessorView &mockDataProcessorView) {
    EXPECT_CALL(mockDataProcessorView, giveUserCritical(_, _)).Times(0);
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _)).Times(0);
  }

  void expectInstrumentIsINTER(MockDataProcessorView &mockDataProcessorView,
                               Cardinality numTimes) {
    if (numTimes.IsSatisfiedByCallCount(0)) {
      // If 0 calls, don't check return value
      EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
          .Times(numTimes);
    } else {
      EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
          .Times(numTimes)
          .WillRepeatedly(Return("INTER"));
    }
  }

  // A list of commonly used input/output workspace names
  std::vector<std::string> m_defaultWorkspaces = {
      "TestWorkspace",  "TOF_12345",
      "TOF_12346",      "IvsQ_binned_TOF_12345",
      "IvsQ_TOF_12345", "IvsQ_binned_TOF_12346",
      "IvsQ_TOF_12346", "IvsQ_TOF_12345_TOF_12346"};

  // Same as above but input workspaces don't have TOF_ prefix
  std::vector<std::string> m_defaultWorkspacesNoPrefix = {
      "TestWorkspace",  "12345",
      "12346",          "IvsQ_binned_TOF_12345",
      "IvsQ_TOF_12345", "IvsQ_binned_TOF_12346",
      "IvsQ_TOF_12346", "IvsQ_TOF_12345_TOF_12346"};

  void checkWorkspacesExistInADS(std::vector<std::string> workspaceNames) {
    for (auto &ws : workspaceNames)
      TS_ASSERT(AnalysisDataService::Instance().doesExist(ws));
  }

  void removeWorkspacesFromADS(std::vector<std::string> workspaceNames) {
    for (auto &ws : workspaceNames)
      AnalysisDataService::Instance().remove(ws);
  }

public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static GenericDataProcessorPresenterTest *createSuite() {
    return new GenericDataProcessorPresenterTest();
  }
  static void destroySuite(GenericDataProcessorPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    DefaultValue<QString>::Set(QString());
    DefaultValue<ColumnOptionsQMap>::Set(ColumnOptionsQMap());
  }

  void tearDown() override {
    DefaultValue<QString>::Clear();
    DefaultValue<ColumnOptionsQMap>::Clear();
  }

  GenericDataProcessorPresenterTest() { FrameworkManager::Instance(); }

  void testConstructor() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    // We don't the view we will handle yet, so none of the methods below
    // should be called
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, _)).Times(0);
    EXPECT_CALL(mockDataProcessorView, addActionsProxy()).Times(0);
    // Constructor
    auto presenter = makeDefaultPresenterNoThread();

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));

    // Check that the presenter updates the whitelist adding columns 'Group'
    // and 'Options'
    auto whitelist = presenter->getWhiteList();
    TS_ASSERT_EQUALS(whitelist.size(), 9);
    TS_ASSERT_EQUALS(whitelist.name(0), "Run(s)");
    TS_ASSERT_EQUALS(whitelist.name(7), "Options");
    TS_ASSERT_EQUALS(whitelist.name(8), "HiddenOptions");
  }

  void testPresenterAcceptsViews() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    auto presenter = makeDefaultPresenter();

    // When the presenter accepts the views, expect the following:
    // Expect that the list of actions is published
    EXPECT_CALL(mockDataProcessorView, addActionsProxy()).Times(Exactly(1));
    // Expect that the list of settings is populated
    EXPECT_CALL(mockDataProcessorView, loadSettings(_)).Times(Exactly(1));
    // Expect that the layout containing pre-processing, processing and
    // post-processing options is created
    EXPECT_CALL(mockDataProcessorView, enableGrouping()).Times(Exactly(1));
    std::vector<QString> stages = {"Pre-process", "Pre-process", "Process",
                                   "Post-process"};
    std::vector<QString> algorithms = {
        "Plus", "CreateTransmissionWorkspaceAuto",
        "ReflectometryReductionOneAuto", "Stitch1DMany"};

    // Expect that the autocompletion hints are populated
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, 7))
        .Times(Exactly(1));
    // Now accept the views
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testNonPostProcessPresenterAcceptsViews() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    auto presenter = makeNonPostProcessPresenter();

    // When the presenter accepts the views, expect the following:
    // Expect that the list of actions is published
    EXPECT_CALL(mockDataProcessorView, addActionsProxy()).Times(Exactly(1));
    // Expect that the list of settings is populated
    EXPECT_CALL(mockDataProcessorView, loadSettings(_)).Times(Exactly(1));
    // Expect that the layout containing pre-processing, processing and
    // post-processing options is created
    EXPECT_CALL(mockDataProcessorView, enableGrouping()).Times(Exactly(0));
    std::vector<QString> stages = {"Pre-process", "Pre-process", "Process",
                                   "Post-process"};
    std::vector<QString> algorithms = {
        "Plus", "CreateTransmissionWorkspaceAuto",
        "ReflectometryReductionOneAuto", "Stitch1DMany"};

    // Expect that the autocompletion hints are populated
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, 7))
        .Times(Exactly(1));
    // Now accept the views
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testSaveNew() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    presenter->notify(DataProcessorPresenter::NewTableFlag);

    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(1),
                               "TestWorkspace");
    presenter->notify(DataProcessorPresenter::SaveFlag);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TestWorkspace"));
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testSaveExisting() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(0),
                               "TestWorkspace");
    presenter->notify(DataProcessorPresenter::SaveFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testSaveAs() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits "save as" but cancels when choosing a name
    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(1));
    presenter->notify(DataProcessorPresenter::SaveAsFlag);

    // The user hits "save as" and and enters "Workspace" for a name
    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(1), "Workspace");
    presenter->notify(DataProcessorPresenter::SaveAsFlag);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));
    ITableWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->columnCount(), 10);

    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testAppendRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits "append row" twice with no rows selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, Exactly(2));
    presenter->notify(DataProcessorPresenter::AppendRowFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has been modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testAppendRowSpecify() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(1);

    // The user hits "append row" twice, with the second row selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, Exactly(2), rowlist);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has been modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testAppendRowSpecifyPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    // The user hits "append row" once, with the second, third, and fourth
    // row selected.
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, Exactly(1), rowlist);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testAppendRowSpecifyGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    // The user hits "append row" once, with the first group selected.
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, Exactly(1), RowList(), grouplist);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testAppendGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits "append row" once, with the first group selected.
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(GroupList()));
    presenter->notify(DataProcessorPresenter::AppendGroupFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testAppendGroupSpecifyPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspaceThreeGroups("TestWorkspace",
                                        presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);
    GroupList grouplist;
    grouplist.insert(0);
    grouplist.insert(1);

    // The user hits "append group" once, with the first and second groups
    // selected.
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter->notify(DataProcessorPresenter::AppendGroupFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 7);
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "2");
    TS_ASSERT_EQUALS(ws->String(6, GroupCol), "2");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testDeleteRowNone() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits "delete row" with no rows selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(RowList()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(0);
    presenter->notify(DataProcessorPresenter::DeleteRowFlag);

    // The user hits save
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has not lost any rows
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testDeleteRowSingle() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(1);

    // The user hits "delete row" with the second row selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(0);
    presenter->notify(DataProcessorPresenter::DeleteRowFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24682");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testDeleteRowPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    // The user hits "delete row" with the first three rows selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::DeleteRowFlag);

    // The user hits save
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check the rows were deleted as expected
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "24682");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testDeleteGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits "delete group" with no groups selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(GroupList()));
    presenter->notify(DataProcessorPresenter::DeleteGroupFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testDeleteGroupPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspaceThreeGroups("TestWorkspace",
                                        presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);
    grouplist.insert(1);

    // The user hits "delete row" with the second row selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter->notify(DataProcessorPresenter::DeleteGroupFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "30000");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "30001");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "2");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "2");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testDeleteAll() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // "delete all" is called with no groups selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(0);
    presenter->notify(DataProcessorPresenter::DeleteAllFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 0);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void expectNotifiedReductionPaused(MockMainPresenter &mockMainPresenter) {
    EXPECT_CALL(mockMainPresenter, confirmReductionPaused());
  }

  void expectNotifiedReductionResumed(MockMainPresenter &mockMainPresenter) {
    EXPECT_CALL(mockMainPresenter, confirmReductionPaused());
  }

  void testProcess() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = makeDefaultPresenterNoThread();
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");
    expectUpdateViewToPausedState(mockDataProcessorView, AtLeast(1));
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), RowList(), grouplist);
    expectUpdateViewToProcessingState(mockDataProcessorView, Exactly(1));
    expectNotebookIsDisabled(mockDataProcessorView, Exactly(1));
    expectNotifiedReductionResumed(mockMainPresenter);
    expectInstrumentIsINTER(mockDataProcessorView, Exactly(2));
    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Check output and tidy up
    checkWorkspacesExistInADS(m_defaultWorkspaces);
    removeWorkspacesFromADS(m_defaultWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessAll() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = makeDefaultPresenterNoThread();
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");
    expectUpdateViewToPausedState(mockDataProcessorView, AtLeast(1));
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);
    grouplist.insert(1);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");
    createTOFWorkspace("TOF_24681", "24681");
    createTOFWorkspace("TOF_24682", "24682");

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, Exactly(0));
    expectUpdateViewToProcessingState(mockDataProcessorView, Exactly(1));
    expectNotebookIsDisabled(mockDataProcessorView, Exactly(1));
    expectInstrumentIsINTER(mockDataProcessorView, Exactly(4));
    expectNotifiedReductionResumed(mockMainPresenter);

    presenter->notify(DataProcessorPresenter::ProcessAllFlag);

    // Check output and tidy up
    auto firstGroupWorkspaces = m_defaultWorkspaces;
    auto secondGroupWorkspaces =
        std::vector<std::string>{"TestWorkspace",  "TOF_24681",
                                 "TOF_24682",      "IvsQ_binned_TOF_24681",
                                 "IvsQ_TOF_24681", "IvsQ_binned_TOF_24682",
                                 "IvsQ_TOF_24682", "IvsQ_TOF_24681_TOF_24682"};

    checkWorkspacesExistInADS(firstGroupWorkspaces);
    checkWorkspacesExistInADS(secondGroupWorkspaces);
    removeWorkspacesFromADS(secondGroupWorkspaces);
    removeWorkspacesFromADS(firstGroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessExitsIfSkipProcessingIsTrue() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");

    auto presenter = makeDefaultPresenterNoThread();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    presenter->skipProcessing();

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // The user hits the "process" button
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1));
    expectUpdateViewToProcessingState(mockDataProcessorView, Exactly(0));
    expectNotebookIsDisabled(mockDataProcessorView, Exactly(0));
    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Tidy up
    removeWorkspacesFromADS(m_defaultWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testTreeUpdatedAfterProcess() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");

    auto presenter = makeDefaultPresenterNoThread();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    auto ws =
        createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    ws->String(0, ThetaCol) = "";
    ws->String(1, ThetaCol) = "";
    ws->String(0, ScaleCol) = "";
    ws->String(1, ScaleCol) = "";
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), RowList(), grouplist);
    presenter->notify(DataProcessorPresenter::ProcessFlag);
    presenter->notify(DataProcessorPresenter::SaveFlag);

    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT(ws->String(0, ThetaCol) != "");
    TS_ASSERT(ws->String(0, ScaleCol) != "");
    TS_ASSERT(ws->String(1, ThetaCol) != "");
    TS_ASSERT(ws->String(1, ScaleCol) != "");

    // Check output and tidy up
    checkWorkspacesExistInADS(m_defaultWorkspaces);
    removeWorkspacesFromADS(m_defaultWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testTreeUpdatedAfterProcessMultiPeriod() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");

    auto presenter = makeDefaultPresenterNoThread();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    auto ws =
        createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    ws->String(0, ThetaCol) = "";
    ws->String(0, ScaleCol) = "";
    ws->String(1, ThetaCol) = "";
    ws->String(1, ScaleCol) = "";
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    createMultiPeriodTOFWorkspace("TOF_12345", "12345");
    createMultiPeriodTOFWorkspace("TOF_12346", "12346");

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), RowList(), grouplist);
    presenter->notify(DataProcessorPresenter::ProcessFlag);
    presenter->notify(DataProcessorPresenter::SaveFlag);

    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(0, ThetaCol), "22.5");
    TS_ASSERT_EQUALS(ws->String(0, ScaleCol), "1");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "22.5");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "1");

    // Check output and tidy up
    checkWorkspacesExistInADS(m_defaultWorkspaces);
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessOnlyRowsSelected() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    auto presenter = makeDefaultPresenterNoThread();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // The user hits the "process" button with the first two rows
    // selected
    // This means we will process the selected rows but we will not
    // post-process them
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), rowlist);
    expectAskUserYesNo(mockDataProcessorView, Exactly(0));
    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Check output and tidy up
    checkWorkspacesExistInADS(m_defaultWorkspaces);
    removeWorkspacesFromADS(m_defaultWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessWithNotebook() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");

    auto presenter = makeDefaultPresenterNoThread();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), RowList(), grouplist);
    expectNotebookIsEnabled(mockDataProcessorView, Exactly(1));
    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Tidy up
    removeWorkspacesFromADS(m_defaultWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testExpandAllGroups() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits the 'Expand All' button
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, expandAll()).Times(1);
    presenter->notify(DataProcessorPresenter::ExpandAllGroupsFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testCollapseAllGroups() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits the 'Expand All' button
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, collapseAll()).Times(1);
    presenter->notify(DataProcessorPresenter::CollapseAllGroupsFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testSelectAll() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // Select all rows / groups
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, selectAll()).Times(1);
    presenter->notify(DataProcessorPresenter::SelectAllFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  /*
   * Test processing workspaces with non-standard names, with
   * and without run_number information in the sample log.
   */
  void testProcessCustomNames() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");

    auto presenter = makeDefaultPresenterNoThread();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    auto ws = createWorkspace("TestWorkspace", presenter->getWhiteList());
    TableRow row = ws->appendRow();
    row << "1"
        << "dataA"
        << "0.7"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "ProcessingInstructions='1'";
    row = ws->appendRow();
    row << "1"
        << "dataB"
        << "2.3"
        << ""
        << "0.13"
        << "2.9"
        << "0.04"
        << "1"
        << "ProcessingInstructions='1'";

    createTOFWorkspace("dataA");
    createTOFWorkspace("dataB");

    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), RowList(), grouplist);
    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspaces were created as expected
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_dataA"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_dataB"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_dataA"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_dataB"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam_TOF_dataA"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam_TOF_dataB"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_dataA_TOF_dataB"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("dataA");
    AnalysisDataService::Instance().remove("dataB");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_dataA");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_dataB");
    AnalysisDataService::Instance().remove("IvsQ_TOF_dataA");
    AnalysisDataService::Instance().remove("IvsQ_TOF_dataB");
    AnalysisDataService::Instance().remove("IvsLam_TOF_dataA");
    AnalysisDataService::Instance().remove("IvsLam_TOF_dataB");
    AnalysisDataService::Instance().remove("IvsQ_TOF_dataA_TOF_dataB");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  std::unique_ptr<GenericDataProcessorPresenter> makeNonPostProcessPresenter() {
    return std::make_unique<GenericDataProcessorPresenter>(
        createReflectometryWhiteList(), createReflectometryPreprocessingStep(),
        createReflectometryProcessor(), DEFAULT_GROUP_NUMBER);
  }

  void testBadWorkspaceType() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    // Wrong types
    ws->addColumn("int", "StitchGroup");
    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "ThetaIn");
    ws->addColumn("str", "TransRun(s)");
    ws->addColumn("str", "Qmin");
    ws->addColumn("str", "Qmax");
    ws->addColumn("str", "dq/q");
    ws->addColumn("str", "Scale");
    ws->addColumn("str", "Options");

    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // We should receive an error
    EXPECT_CALL(mockDataProcessorView, giveUserCritical(_, _)).Times(1);

    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testBadWorkspaceLength() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // Because we to open twice, get an error twice
    EXPECT_CALL(mockDataProcessorView, giveUserCritical(_, _)).Times(2);
    expectGetWorkspace(mockDataProcessorView, Exactly(2), "TestWorkspace");

    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "StitchGroup");
    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "ThetaIn");
    ws->addColumn("str", "TransRun(s)");
    ws->addColumn("str", "Qmin");
    ws->addColumn("str", "Qmax");
    ws->addColumn("str", "dq/q");
    ws->addColumn("str", "Scale");
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    // Try to open with too few columns
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    ws->addColumn("str", "OptionsA");
    ws->addColumn("str", "OptionsB");
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    // Try to open with too many columns
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPromptSaveAfterAppendRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // User hits "append row"
    expectGetSelection(mockDataProcessorView, Exactly(1));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // The user will decide not to discard their changes
    expectAskUserYesNo(mockDataProcessorView, Exactly(1));

    // Then hits "new table" without having saved
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(1), "Workspace");
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    expectAskUserYesNo(mockDataProcessorView, Exactly(0));
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPromptSaveAfterAppendGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // User hits "append group"
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(GroupList()));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::AppendGroupFlag);

    // The user will decide not to discard their changes
    expectAskUserYesNo(mockDataProcessorView, Exactly(1));

    // Then hits "new table" without having saved
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(1), "Workspace");
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    expectAskUserYesNo(mockDataProcessorView, Exactly(0));
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPromptSaveAfterDeleteRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // User hits "append row" a couple of times
    expectGetSelection(mockDataProcessorView, Exactly(2));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // The user saves
    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(1), "Workspace");
    presenter->notify(DataProcessorPresenter::SaveFlag);

    //...then deletes the 2nd row
    RowList rowlist;
    rowlist[0].insert(1);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::DeleteRowFlag);

    // The user will decide not to discard their changes when asked
    expectAskUserYesNo(mockDataProcessorView, Exactly(1));

    // Then hits "new table" without having saved
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    expectAskUserYesNo(mockDataProcessorView, Exactly(0));
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPromptSaveAfterDeleteGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // User hits "append group" a couple of times
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(GroupList()));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::AppendGroupFlag);
    presenter->notify(DataProcessorPresenter::AppendGroupFlag);

    // The user saves
    expectAskUserWorkspaceName(mockDataProcessorView, Exactly(1), "Workspace");
    presenter->notify(DataProcessorPresenter::SaveFlag);

    //...then deletes the 2nd row
    GroupList grouplist;
    grouplist.insert(1);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::DeleteGroupFlag);

    // The user will decide not to discard their changes when asked
    expectAskUserYesNo(mockDataProcessorView, Exactly(1));

    // Then hits "new table" without having saved
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    expectAskUserYesNo(mockDataProcessorView, Exactly(0));
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPromptSaveAndDiscard() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // User hits "append row" a couple of times
    expectGetSelection(mockDataProcessorView, Exactly(2));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // Then hits "new table", and decides to discard
    expectAskUserYesNo(mockDataProcessorView, Exactly(1), true);
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    // These next two times they don't get prompted - they have a new table
    presenter->notify(DataProcessorPresenter::NewTableFlag);
    presenter->notify(DataProcessorPresenter::NewTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPromptSaveOnOpen() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());

    // User hits "append row"
    expectGetSelection(mockDataProcessorView, Exactly(1));
    presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
    presenter->notify(DataProcessorPresenter::AppendRowFlag);

    // and tries to open a workspace, but gets prompted and decides not to
    // discard
    expectAskUserYesNo(mockDataProcessorView, Exactly(1));
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // the user does it again, but discards
    expectAskUserYesNo(mockDataProcessorView, Exactly(1), true);
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    // the user does it one more time, and is not prompted
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    expectAskUserYesNo(mockDataProcessorView, Exactly(0));
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testExpandSelection() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    auto ws = createWorkspace("TestWorkspace", presenter->getWhiteList());
    TableRow row = ws->appendRow();
    row << "0"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 0
    row = ws->appendRow();
    row << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 1
    row = ws->appendRow();
    row << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 2
    row = ws->appendRow();
    row << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 3
    row = ws->appendRow();
    row << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 4
    row = ws->appendRow();
    row << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 5
    row = ws->appendRow();
    row << "3"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 6
    row = ws->appendRow();
    row << "4"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 7
    row = ws->appendRow();
    row << "4"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 8
    row = ws->appendRow();
    row << "5"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 9

    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList selection;
    GroupList expected;

    selection[0].insert(0);
    expected.insert(0);

    // With row 0 selected, we shouldn't expand at all
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter->notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With 0,1 selected, we should finish with groups 0,1 selected
    selection.clear();
    selection[0].insert(0);
    selection[1].insert(0);

    expected.clear();
    expected.insert(0);
    expected.insert(1);

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter->notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With 1,6 selected, we should finish with groups 1,3 selected
    selection.clear();
    selection[1].insert(0);
    selection[3].insert(0);

    expected.clear();
    expected.insert(1);
    expected.insert(3);

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter->notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With 4,8 selected, we should finish with groups 2,4 selected
    selection.clear();
    selection[2].insert(1);
    selection[4].insert(2);

    expected.clear();
    expected.insert(2);
    expected.insert(4);

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter->notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With nothing selected, we should finish with nothing selected
    selection.clear();
    expected.clear();

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(_)).Times(0);
    presenter->notify(DataProcessorPresenter::ExpandSelectionFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testGroupRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    auto ws = createWorkspace("TestWorkspace", presenter->getWhiteList());
    TableRow row = ws->appendRow();
    row << "0"
        << "0"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 0
    row = ws->appendRow();
    row << "0"
        << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 1
    row = ws->appendRow();
    row << "0"
        << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 2
    row = ws->appendRow();
    row << "0"
        << "3"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 3

    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList selection;
    selection[0].insert(0);
    selection[0].insert(1);

    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(GroupList()));
    presenter->notify(DataProcessorPresenter::GroupRowsFlag);
    presenter->notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has been modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "");
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "2");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "3");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "0");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testGroupRowsNothingSelected() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    auto ws = createWorkspace("TestWorkspace", presenter->getWhiteList());
    TableRow row = ws->appendRow();
    row << "0"
        << "0"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 0
    row = ws->appendRow();
    row << "0"
        << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 1
    row = ws->appendRow();
    row << "0"
        << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 2
    row = ws->appendRow();
    row << "0"
        << "3"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 3

    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(RowList()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(0);
    presenter->notify(DataProcessorPresenter::GroupRowsFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testClearRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    // The user hits "clear selected" with the second and third rows selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::ClearSelectedFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the group ids have been set correctly
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");

    // Make sure the selected rows are clear
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "");
    TS_ASSERT_EQUALS(ws->String(2, ThetaCol), "");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "");
    TS_ASSERT_EQUALS(ws->String(2, TransCol), "");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "");
    TS_ASSERT_EQUALS(ws->String(2, QMinCol), "");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "");
    TS_ASSERT_EQUALS(ws->String(2, QMaxCol), "");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "");
    TS_ASSERT_EQUALS(ws->String(2, DQQCol), "");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "");
    TS_ASSERT_EQUALS(ws->String(2, ScaleCol), "");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testCopyRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(1);

    const auto expected = QString(
        "0\t12346\t1.5\t\t0.13\t2.9\t0.04\t1\tProcessingInstructions='1'\t");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::CopySelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testCopyEmptySelection() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(QString())).Times(1);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(RowList()));
    presenter->notify(DataProcessorPresenter::CopySelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testCopyRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);
    rowlist[1].insert(1);

    const auto expected = QString(
        "0\t12345\t0.5\t\t0.1\t1.6\t0.04\t1\tProcessingInstructions='1'\t\n"
        "0\t12346\t1.5\t\t0.13\t2.9\t0.04\t1\tProcessingInstructions='1'\t\n"
        "1\t24681\t0.5\t\t0.1\t1.6\t0.04\t1\tProcessingInstructions='1'\t\n"
        "1\t24682\t1.5\t\t0.13\t2.9\t0.04\t1\tProcessingInstructions='1'\t");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::CopySelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testCutRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(1);

    const auto expected = QString(
        "0\t12346\t1.5\t\t0.13\t2.9\t0.04\t1\tProcessingInstructions='1'\t");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::CutSelectedFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24682");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testCutRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    const auto expected = QString(
        "0\t12345\t0.5\t\t0.1\t1.6\t0.04\t1\tProcessingInstructions='1'\t\n"
        "0\t12346\t1.5\t\t0.13\t2.9\t0.04\t1\tProcessingInstructions='1'\t\n"
        "1\t24681\t0.5\t\t0.1\t1.6\t0.04\t1\tProcessingInstructions='1'\t");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::CutSelectedFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    // Check the only unselected row is left behind
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "24682");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPasteRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(1);

    const auto clipboard =
        QString("6\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc\tdef");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(1, OptionsCol), "abc");
    TS_ASSERT_EQUALS(ws->String(1, HiddenOptionsCol), "def");

    // Row is going to be pasted into the group where row in clipboard
    // belongs, i.e. group 0
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPasteNewRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    const auto clipboard =
        QString("1\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc\tdef");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(RowList()));
    presenter->notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(4, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(4, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(4, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(4, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(4, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(4, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, OptionsCol), "abc");
    TS_ASSERT_EQUALS(ws->String(4, HiddenOptionsCol), "def");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPasteRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    const auto clipboard =
        QString("6\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc\tdef\n"
                "2\t345\t2.7\t123\t2.1\t4.3\t2.17\t3\tdef\tabc");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter->notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, OptionsCol), "abc");
    TS_ASSERT_EQUALS(ws->String(1, HiddenOptionsCol), "def");

    TS_ASSERT_EQUALS(ws->String(2, RunCol), "345");
    TS_ASSERT_EQUALS(ws->String(2, ThetaCol), "2.7");
    TS_ASSERT_EQUALS(ws->String(2, TransCol), "123");
    TS_ASSERT_EQUALS(ws->String(2, QMinCol), "2.1");
    TS_ASSERT_EQUALS(ws->String(2, QMaxCol), "4.3");
    TS_ASSERT_EQUALS(ws->String(2, DQQCol), "2.17");
    TS_ASSERT_EQUALS(ws->String(2, ScaleCol), "3");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(2, OptionsCol), "def");
    TS_ASSERT_EQUALS(ws->String(2, HiddenOptionsCol), "abc");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPasteNewRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    const auto clipboard =
        QString("1\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc\tzzz\n"
                "1\t345\t2.7\t123\t2.1\t4.3\t2.17\t3\tdef\tyyy");

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(RowList()));
    presenter->notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter->notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(4, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(4, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(4, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(4, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(4, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(4, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, OptionsCol), "abc");
    TS_ASSERT_EQUALS(ws->String(4, HiddenOptionsCol), "zzz");

    TS_ASSERT_EQUALS(ws->String(5, RunCol), "345");
    TS_ASSERT_EQUALS(ws->String(5, ThetaCol), "2.7");
    TS_ASSERT_EQUALS(ws->String(5, TransCol), "123");
    TS_ASSERT_EQUALS(ws->String(5, QMinCol), "2.1");
    TS_ASSERT_EQUALS(ws->String(5, QMaxCol), "4.3");
    TS_ASSERT_EQUALS(ws->String(5, DQQCol), "2.17");
    TS_ASSERT_EQUALS(ws->String(5, ScaleCol), "3");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(5, OptionsCol), "def");
    TS_ASSERT_EQUALS(ws->String(5, HiddenOptionsCol), "yyy");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPasteEmptyClipboard() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // Empty clipboard
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(QString()));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    presenter->notify(DataProcessorPresenter::PasteSelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPasteToNonexistentGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    // Empty clipboard
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return("1\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc\t"));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillOnce(Return(RowList()));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::PasteSelectedFlag));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testImportTable() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    EXPECT_CALL(
        mockDataProcessorView,
        runPythonAlgorithm(QString("try:\n  algm = LoadTBLDialog()\n  print("
                                   "algm.getPropertyValue(\"OutputWorkspace\"))"
                                   "\nexcept:\n  pass\n")));
    presenter->notify(DataProcessorPresenter::ImportTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testExportTable() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    EXPECT_CALL(mockDataProcessorView,
                runPythonAlgorithm(QString(
                    "try:\n  algm = SaveTBLDialog()\nexcept:\n  pass\n")));
    presenter->notify(DataProcessorPresenter::ExportTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPlotRowWarn() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    createTOFWorkspace("TOF_12345", "12345");
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");

    // We should be warned
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    RowList rowlist;
    rowlist[0].insert(0);

    // We should be warned
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _));
    // The user hits "plot rows" with the first row selected
    expectGetSelection(mockDataProcessorView, Exactly(1), rowlist);
    presenter->notify(DataProcessorPresenter::PlotRowFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPlotEmptyRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    RowList rowlist;
    rowlist[0].insert(0);
    expectGetSelection(mockDataProcessorView, Exactly(2), rowlist);
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _));
    // Append an empty row to our table
    presenter->notify(DataProcessorPresenter::AppendRowFlag);
    // Attempt to plot the empty row (should result in critical warning)
    presenter->notify(DataProcessorPresenter::PlotRowFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPlotGroupWithEmptyRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    createTOFWorkspace("TOF_12345", "12345");
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    RowList rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    GroupList grouplist;
    grouplist.insert(0);
    expectGetSelection(mockDataProcessorView, Exactly(2), rowlist, grouplist);
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _));
    // Open up our table with one row
    presenter->notify(DataProcessorPresenter::OpenTableFlag);
    // Append an empty row to the table
    presenter->notify(DataProcessorPresenter::AppendRowFlag);
    // Attempt to plot the group (should result in critical warning)
    presenter->notify(DataProcessorPresenter::PlotGroupFlag);
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPlotGroupWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    // We should be warned
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _));
    // The user hits "plot groups" with the first row selected
    expectGetSelection(mockDataProcessorView, Exactly(1), RowList(), grouplist);
    presenter->notify(DataProcessorPresenter::PlotGroupFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testWorkspaceNamesNoTrans() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    auto row0 =
        makeRowData({"12345", "0.5", "", "0.1", "0.3", "0.04", "1", "", ""});
    auto row1 =
        makeRowData({"12346", "0.5", "", "0.1", "0.3", "0.04", "1", "", ""});
    GroupData group = {{0, row0}, {1, row1}};

    // Find and cache the reduced workspace names
    row0->setReducedName(presenter->getReducedWorkspaceName(row0));
    row1->setReducedName(presenter->getReducedWorkspaceName(row1));

    // Test the names of the reduced workspaces
    TS_ASSERT_EQUALS(row0->reducedName().toStdString(), "TOF_12345");
    TS_ASSERT_EQUALS(row1->reducedName().toStdString(), "TOF_12346");
    // Test the names of the post-processed ws
    TS_ASSERT_EQUALS(
        presenter->getPostprocessedWorkspaceName(group).toStdString(),
        "IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testWorkspaceNamesWithTrans() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();

    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    auto row0 = makeRowData(
        {"12345", "0.5", "11115", "0.1", "0.3", "0.04", "1", "", ""});
    auto row1 = makeRowData(
        {"12346", "0.5", "11116", "0.1", "0.3", "0.04", "1", "", ""});
    GroupData group = {{0, row0}, {1, row1}};

    // Find and cache the reduced workspace names
    row0->setReducedName(presenter->getReducedWorkspaceName(row0));
    row1->setReducedName(presenter->getReducedWorkspaceName(row1));

    // Test the names of the reduced workspaces
    TS_ASSERT_EQUALS(row0->reducedName().toStdString(),
                     "TOF_12345_TRANS_11115");
    TS_ASSERT_EQUALS(row1->reducedName().toStdString(),
                     "TOF_12346_TRANS_11116");
    // Test the names of the post-processed ws
    TS_ASSERT_EQUALS(
        presenter->getPostprocessedWorkspaceName(group).toStdString(),
        "IvsQ_TOF_12345_TRANS_11115_TOF_12346_TRANS_11116");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testWorkspaceNamesWithMultipleTrans() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    // Test transmission run list separated by both comma and plus symbol
    auto row0 = makeRowData(
        {"12345", "0.5", "11115,11116", "0.1", "0.3", "0.04", "1", "", ""});
    auto row1 = makeRowData(
        {"12346", "0.5", "11115+11116", "0.1", "0.3", "0.04", "1", "", ""});
    GroupData group = {{0, row0}, {1, row1}};

    // Find and cache the reduced workspace names
    row0->setReducedName(presenter->getReducedWorkspaceName(row0));
    row1->setReducedName(presenter->getReducedWorkspaceName(row1));

    // Test the names of the reduced workspaces
    TS_ASSERT_EQUALS(row0->reducedName().toStdString(),
                     "TOF_12345_TRANS_11115_11116");
    TS_ASSERT_EQUALS(row1->reducedName().toStdString(),
                     "TOF_12346_TRANS_11115_11116");
    // Test the names of the post-processed ws
    TS_ASSERT_EQUALS(
        presenter->getPostprocessedWorkspaceName(group).toStdString(),
        "IvsQ_TOF_12345_TRANS_11115_11116_TOF_12346_TRANS_11115_11116");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testWorkspaceNameWrongData() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;

    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);

    auto row0 = makeRowData({"12345", "0.5"});
    auto row1 = makeRowData({"12346", "0.5"});
    GroupData group = {{0, row0}, {1, row1}};

    // Test the names of the reduced workspaces
    TS_ASSERT_THROWS_ANYTHING(presenter->getReducedWorkspaceName(row0));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  /// Tests the reduction when no pre-processing algorithms are given

  void testProcessNoPreProcessing() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    expectGetOptions(mockMainPresenter, Exactly(1), "Params = \"0.1\"");

    // We don't know the view we will handle yet, so none of the methods below
    // should be called
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, _)).Times(0);
    // Constructor (no pre-processing)

    GenericDataProcessorPresenterNoThread presenter(
        createReflectometryWhiteList(), createReflectometryProcessor(),
        createReflectometryPostprocessor(), DEFAULT_GROUP_NUMBER);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));

    // Check that the presenter has updated the whitelist adding columns 'Group'
    // and 'Options'
    auto whitelist = presenter.getWhiteList();
    TS_ASSERT_EQUALS(whitelist.size(), 9);
    TS_ASSERT_EQUALS(whitelist.name(0), "Run(s)");
    TS_ASSERT_EQUALS(whitelist.name(7), "Options");

    // When the presenter accepts the views, expect the following:
    // Expect that the list of settings is populated
    EXPECT_CALL(mockDataProcessorView, loadSettings(_)).Times(Exactly(1));
    // Expect that the autocompletion hints are populated
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, 7))
        .Times(Exactly(1));
    // Now accept the views
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    GroupList grouplist;
    grouplist.insert(0);

    createTOFWorkspace("12345", "12345");
    createTOFWorkspace("12346", "12346");

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), RowList(), grouplist);
    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output and tidy up
    checkWorkspacesExistInADS(m_defaultWorkspacesNoPrefix);
    removeWorkspacesFromADS(m_defaultWorkspacesNoPrefix);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotRowPythonCode() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    auto mockTreeManager = std::make_unique<MockTreeManager>();
    auto *mockTreeManager_ptr = mockTreeManager.get();
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->acceptTreeManager(std::move(mockTreeManager));

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);
    createTOFWorkspace("IvsQ_binned_TOF_12345", "12345");
    createTOFWorkspace("IvsQ_binned_TOF_12346", "12346");

    // Set up the expected tree data to be returned in the selection
    auto row0 = makeRowData({"12345"});
    auto row1 = makeRowData({"12346"});
    GroupData group = {{0, row0}, {1, row1}};
    TreeData tree = {{0, group}};

    // The user hits "plot rows" with the first row selected
    expectNoWarningsOrErrors(mockDataProcessorView);

    EXPECT_CALL(*mockTreeManager_ptr, selectedData(false))
        .Times(1)
        .WillOnce(Return(tree));

    auto pythonCode = QString(
        "base_graph = None\nbase_graph = "
        "plotSpectrum(\"IvsQ_binned_TOF_12345\", 0, True, window = "
        "base_graph)\nbase_graph = plotSpectrum(\"IvsQ_binned_TOF_12346\", 0, "
        "True, window = base_graph)\nbase_graph.activeLayer().logLogAxes()\n");

    EXPECT_CALL(mockDataProcessorView, runPythonAlgorithm(pythonCode)).Times(1);
    presenter->notify(DataProcessorPresenter::PlotRowFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testPlotGroupPythonCode() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    auto mockTreeManager = std::make_unique<MockTreeManager>();
    auto *mockTreeManager_ptr = mockTreeManager.get();
    auto presenter = makeDefaultPresenter();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->acceptTreeManager(std::move(mockTreeManager));

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter->notify(DataProcessorPresenter::OpenTableFlag);
    createTOFWorkspace("IvsQ_TOF_12345_TOF_12346");

    // Set up the expected tree data to be returned in the selection
    auto row0 = makeRowData({"12345"});
    auto row1 = makeRowData({"12346"});
    GroupData group = {{0, row0}, {1, row1}};
    TreeData tree = {{0, group}};

    // The user hits "plot rows" with the first row selected
    expectNoWarningsOrErrors(mockDataProcessorView);

    EXPECT_CALL(*mockTreeManager_ptr, selectedData(false))
        .Times(1)
        .WillOnce(Return(tree));

    auto pythonCode =
        QString("base_graph = None\nbase_graph = "
                "plotSpectrum(\"IvsQ_TOF_12345_TOF_12346\", 0, True, window = "
                "base_graph)\nbase_graph.activeLayer().logLogAxes()\n");

    EXPECT_CALL(mockDataProcessorView, runPythonAlgorithm(pythonCode)).Times(1);
    presenter->notify(DataProcessorPresenter::PlotGroupFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testNoPostProcessing() {
    // Test very basic functionality of the presenter when no post-processing
    // algorithm is defined

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    GenericDataProcessorPresenterFriend presenter(
        createReflectometryWhiteList(), createReflectometryProcessor(),
        DEFAULT_GROUP_NUMBER);
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);

    // Calls that should throw
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::AppendGroupFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::DeleteGroupFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::GroupRowsFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::ExpandSelectionFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::PlotGroupFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.getPostprocessedWorkspaceName(GroupData()));
  }

  void testPostprocessMap() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    expectGetOptions(mockMainPresenter, Exactly(1), "Params='-0.10'");

    std::map<QString, QString> postprocesssMap = {{"dQ/Q", "Params"}};
    GenericDataProcessorPresenterNoThread presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessingStep(),
        createReflectometryProcessor(), createReflectometryPostprocessor(),
        DEFAULT_GROUP_NUMBER, postprocesssMap);
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // Open a table
    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    expectGetWorkspace(mockDataProcessorView, Exactly(1), "TestWorkspace");
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    createTOFWorkspace("12345", "12345");
    createTOFWorkspace("12346", "12346");

    GroupList grouplist;
    grouplist.insert(0);

    // The user hits the "process" button with the first group selected
    expectNoWarningsOrErrors(mockDataProcessorView);
    expectGetSelection(mockDataProcessorView, AtLeast(1), RowList(), grouplist);
    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspace was stitched with params = '-0.04'
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    MatrixWorkspace_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "IvsQ_TOF_12345_TOF_12346");
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[0], 0.13860, 1e-5);
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[1], 0.14415, 1e-5);
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[2], 0.14991, 1e-5);
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[3], 0.15591, 1e-5);

    // Check output and tidy up
    checkWorkspacesExistInADS(m_defaultWorkspacesNoPrefix);
    removeWorkspacesFromADS(m_defaultWorkspacesNoPrefix);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPauseReduction() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    auto presenter = makeDefaultPresenter();

    expectUpdateViewToPausedState(mockDataProcessorView, AtLeast(1));
    // Now accept the views
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    // User hits the 'pause' button
    expectNoWarningsOrErrors(mockDataProcessorView);
    // The widget states are not updated immediately (only on confirm)
    expectUpdateViewToPausedState(mockDataProcessorView, Exactly(0));
    EXPECT_CALL(mockMainPresenter, pause()).Times(1);
    presenter->notify(DataProcessorPresenter::PauseFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testInstrumentList() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    GenericDataProcessorPresenter presenter(createReflectometryWhiteList(),
                                            createReflectometryProcessor(),
                                            DEFAULT_GROUP_NUMBER);
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);

    EXPECT_CALL(mockDataProcessorView,
                setInstrumentList(
                    QString::fromStdString("INTER,SURF,POLREF,OFFSPEC,CRISP"),
                    QString::fromStdString("INTER")))
        .Times(1);
    presenter.setInstrumentList(
        QStringList{"INTER", "SURF", "POLREF", "OFFSPEC", "CRISP"}, "INTER");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }
};
#endif /* MANTID_MANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERTEST_H */
