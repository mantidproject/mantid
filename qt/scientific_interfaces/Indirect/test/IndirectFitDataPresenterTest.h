// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTFITDATAPRESENTERTEST_H_
#define MANTIDQT_INDIRECTFITDATAPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IIndirectFitDataView.h"
#include "IndirectDataTablePresenter.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFittingModel.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;
using IDAWorkspaceIndex = MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

namespace {

std::unique_ptr<QTableWidget> createEmptyTableWidget(int columns, int rows) {
  auto table = std::make_unique<QTableWidget>(columns, rows);
  for (auto column = 0; column < columns; ++column)
    for (auto row = 0; row < rows; ++row)
      table->setItem(row, column, new QTableWidgetItem("item"));
  return table;
}

struct TableItem {
  TableItem(std::string const &value) : m_str(value), m_dbl(0.0) {}
  TableItem(double const &value)
      : m_str(QString::number(value, 'g', 16).toStdString()), m_dbl(value) {}

  std::string const &asString() const { return m_str; }
  double const &asDouble() const { return m_dbl; }

  bool operator==(std::string const &value) const {
    return this->asString() == value;
  }

private:
  std::string m_str;
  double m_dbl;
};

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIIndirectFitDataView : public IIndirectFitDataView {
public:
  /// Signals
  void emitSampleLoaded(QString const &name) { emit sampleLoaded(name); }

  /// Public Methods
  MOCK_CONST_METHOD0(getDataTable, QTableWidget *());
  MOCK_CONST_METHOD0(isMultipleDataTabSelected, bool());
  MOCK_CONST_METHOD0(isResolutionHidden, bool());
  MOCK_METHOD1(setResolutionHidden, void(bool hide));
  MOCK_METHOD0(disableMultipleDataTab, void());

  MOCK_CONST_METHOD0(getSelectedSample, std::string());
  MOCK_CONST_METHOD0(getSelectedResolution, std::string());

  MOCK_CONST_METHOD0(getSampleWSSuffices, QStringList());
  MOCK_CONST_METHOD0(getSampleFBSuffices, QStringList());
  MOCK_CONST_METHOD0(getResolutionWSSuffices, QStringList());
  MOCK_CONST_METHOD0(getResolutionFBSuffices, QStringList());

  MOCK_METHOD1(setSampleWSSuffices, void(QStringList const &suffices));
  MOCK_METHOD1(setSampleFBSuffices, void(QStringList const &suffices));
  MOCK_METHOD1(setResolutionWSSuffices, void(QStringList const &suffices));
  MOCK_METHOD1(setResolutionFBSuffices, void(QStringList const &suffices));

  MOCK_CONST_METHOD0(isSampleWorkspaceSelectorVisible, bool());
  MOCK_METHOD1(setSampleWorkspaceSelectorIndex,
               void(QString const &workspaceName));

  MOCK_METHOD1(readSettings, void(QSettings const &settings));
  MOCK_METHOD1(validate, UserInputValidator &(UserInputValidator &validator));
  MOCK_METHOD1(setXRange, void(std::pair<double, double> const &range));

  /// Public slots
  MOCK_METHOD1(displayWarning, void(std::string const &warning));
  MOCK_METHOD1(setStartX, void(double));
  MOCK_METHOD1(setEndX, void(double));
};

/// Mock object to mock the model
class MockIndirectFitDataModel : public IndirectFittingModel {
public:
  using IndirectFittingModel::addWorkspace;

  /// Public Methods
  MOCK_CONST_METHOD1(hasWorkspace, bool(std::string const &workspaceName));

  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(numberOfWorkspaces, DatasetIndex());

  MOCK_METHOD1(addWorkspace, void(std::string const &workspaceName));
  MOCK_CONST_METHOD2(getDomainIndex,
               SpectrumRowIndex(DatasetIndex, IDAWorkspaceIndex));

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(DatasetIndex index,
                                  IDAWorkspaceIndex spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };

  std::vector<std::string> getSpectrumDependentAttributes() const override {
    return {};
  };
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectFitDataPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectFitDataPresenterTest() { FrameworkManager::Instance(); }

  static IndirectFitDataPresenterTest *createSuite() {
    return new IndirectFitDataPresenterTest();
  }

  static void destroySuite(IndirectFitDataPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockIIndirectFitDataView>>();
    m_model = std::make_unique<NiceMock<MockIndirectFitDataModel>>();
    m_table = createEmptyTableWidget(5, 5);

    SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(5));
    m_model->addWorkspace("WorkspaceName");
    ON_CALL(*m_model, numberOfWorkspaces())
        .WillByDefault(Return(DatasetIndex{1}));

    m_dataTablePresenter = std::make_unique<IndirectDataTablePresenter>(
        std::move(m_model.get()), std::move(m_table.get()));
    m_dataTablePresenter->addData(DatasetIndex{0});

    m_presenter = std::make_unique<IndirectFitDataPresenter>(
        std::move(m_model.get()), std::move(m_view.get()),
        std::move(m_dataTablePresenter));
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    deleteSetup();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful mock object instantiation
  ///----------------------------------------------------------------------

  void test_that_the_model_has_been_instantiated_correctly() {
    ON_CALL(*m_model, isMultiFit()).WillByDefault(Return(false));

    EXPECT_CALL(*m_model, isMultiFit()).Times(1).WillOnce(Return(false));

    m_model->isMultiFit();
  }

  void test_that_the_view_has_been_instantiated_correctly() {
    std::string const sampleName("SampleName_red");
    ON_CALL(*m_view, getSelectedSample()).WillByDefault(Return(sampleName));

    EXPECT_CALL(*m_view, getSelectedSample())
        .Times(1)
        .WillOnce(Return(sampleName));

    m_view->getSelectedSample();
  }

  void
  test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_view_and_model() {
    ON_CALL(*m_view, isMultipleDataTabSelected()).WillByDefault(Return(true));
    ON_CALL(*m_model, numberOfWorkspaces())
        .WillByDefault(Return(DatasetIndex{2}));

    Expectation isMultipleData =
        EXPECT_CALL(*m_view, isMultipleDataTabSelected())
            .Times(1)
            .WillOnce(Return(true));

    m_presenter->updateSpectraInTable(DatasetIndex{0});
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void
  test_that_the_sampleLoaded_signal_will_add_the_loaded_workspace_to_the_model() {
    std::string const workspaceName("WorkspaceName2");
    m_ads->addOrReplace(workspaceName, createWorkspace(5));

    EXPECT_CALL(*m_model, addWorkspace(workspaceName)).Times(Exactly(1));

    m_view->emitSampleLoaded(QString::fromStdString(workspaceName));
  }

  void
  test_that_setSampleWSSuffices_will_set_the_sample_workspace_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};

    EXPECT_CALL(*m_view, setSampleWSSuffices(suffices)).Times(Exactly(1));

    m_presenter->setSampleWSSuffices(suffices);
  }

  void
  test_that_setSampleFBSuffices_will_set_the_sample_file_browser_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};

    EXPECT_CALL(*m_view, setSampleFBSuffices(suffices)).Times(Exactly(1));

    m_presenter->setSampleFBSuffices(suffices);
  }

  void
  test_that_setResolutionWSSuffices_will_set_the_resolution_workspace_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};

    EXPECT_CALL(*m_view, setResolutionWSSuffices(suffices)).Times(Exactly(1));

    m_presenter->setResolutionWSSuffices(suffices);
  }

  void
  test_that_setResolutionFBSuffices_will_set_the_resolution_file_browser_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};

    EXPECT_CALL(*m_view, setResolutionFBSuffices(suffices)).Times(Exactly(1));

    m_presenter->setResolutionFBSuffices(suffices);
  }

  void
  test_that_setStartX_will_alter_the_relevant_startX_column_in_the_data_table() {
    TableItem const startX(2.3);
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .WillByDefault(Return(SpectrumRowIndex{0}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{1}))
        .WillByDefault(Return(SpectrumRowIndex{1}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{2}))
        .WillByDefault(Return(SpectrumRowIndex{2}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{3}))
        .WillByDefault(Return(SpectrumRowIndex{3}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{4}))
        .WillByDefault(Return(SpectrumRowIndex{4}));

    m_presenter->setStartX(startX.asDouble(), DatasetIndex{0},
                           IDAWorkspaceIndex{0});
    m_presenter->setStartX(startX.asDouble(), DatasetIndex{0},
                           IDAWorkspaceIndex{1});
    m_presenter->setStartX(startX.asDouble(), DatasetIndex{0},
                           IDAWorkspaceIndex{2});
    m_presenter->setStartX(startX.asDouble(), DatasetIndex{0},
                           IDAWorkspaceIndex{3});
    m_presenter->setStartX(startX.asDouble(), DatasetIndex{0},
                           IDAWorkspaceIndex{4});

    assertValueIsGlobal(START_X_COLUMN, startX);
  }

  void
  test_that_setEndX_will_alter_the_relevant_endX_column_in_the_data_table() {
    TableItem const endX(5.5);
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .WillByDefault(Return(SpectrumRowIndex{0}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{1}))
        .WillByDefault(Return(SpectrumRowIndex{1}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{2}))
        .WillByDefault(Return(SpectrumRowIndex{2}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{3}))
        .WillByDefault(Return(SpectrumRowIndex{3}));
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{4}))
        .WillByDefault(Return(SpectrumRowIndex{4}));

    m_presenter->setEndX(endX.asDouble(), DatasetIndex{0},
                         IDAWorkspaceIndex{0});
    m_presenter->setEndX(endX.asDouble(), DatasetIndex{0},
                         IDAWorkspaceIndex{1});
    m_presenter->setEndX(endX.asDouble(), DatasetIndex{0},
                         IDAWorkspaceIndex{2});
    m_presenter->setEndX(endX.asDouble(), DatasetIndex{0},
                         IDAWorkspaceIndex{3});
    m_presenter->setEndX(endX.asDouble(), DatasetIndex{0},
                         IDAWorkspaceIndex{4});

    assertValueIsGlobal(END_X_COLUMN, endX);
  }

  void
  test_that_the_setExcludeRegion_slot_will_alter_the_relevant_excludeRegion_column_in_the_table() {
    TableItem const excludeRegion("2-3");

    m_presenter->setExclude(excludeRegion.asString(), DatasetIndex{0},
                            IDAWorkspaceIndex{0});

    assertValueIsGlobal(EXCLUDE_REGION_COLUMN, excludeRegion);
  }

  void test_that_loadSettings_will_read_the_settings_from_the_view() {
    QSettings settings;
    settings.beginGroup("/ISettings");

    EXPECT_CALL(*m_view, readSettings(_)).Times(Exactly(1));

    m_presenter->loadSettings(settings);
  }

  void test_that_replaceHandle_will_check_if_the_model_has_a_workspace() {
    std::string const workspacename("DummyName");

    EXPECT_CALL(*m_model, hasWorkspace(workspacename)).Times(Exactly(1));

    m_presenter->replaceHandle(workspacename, createWorkspace(5));
  }

private:
  void deleteSetup() {
    m_presenter.reset();
    m_model.reset();
    m_view.reset();

    m_dataTablePresenter.reset();
    m_table.reset();
  }

  void assertValueIsGlobal(int column, TableItem const &value) const {
    for (auto row = 0; row < m_table->rowCount(); ++row) {
      auto const item = getTableItem(row, column);
      TS_ASSERT_EQUALS(value, item);
    }
  }

  std::string getTableItem(int row, int column) const {
    return m_table->item(row, column)->text().toStdString();
  }

  std::unique_ptr<QTableWidget> m_table;
  std::unique_ptr<IndirectDataTablePresenter> m_dataTablePresenter;

  std::unique_ptr<MockIIndirectFitDataView> m_view;
  std::unique_ptr<MockIndirectFitDataModel> m_model;
  std::unique_ptr<IndirectFitDataPresenter> m_presenter;

  SetUpADSWithWorkspace *m_ads;
};

#endif
