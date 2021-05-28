// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "FqFitDataPresenter.h"
#include "FqFitModel.h"
#include "IndirectFitDataView.h"
#include "IndirectFunctionBrowser/SingleFunctionTemplateBrowser.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

namespace {

QString const PARAMETER_TYPE_LABEL("Fit Parameter:");
QString const PARAMETER_LABEL("Width:");

QStringList getFqFitParameters() {
  QStringList parameters;
  parameters << "f1.f1.FWHM"
             << "f2.f1.FWHM";
  return parameters;
}

QStringList getFqFitParameterTypes() {
  QStringList parameterTypes;
  parameterTypes << "Width"
                 << "EISF";
  return parameterTypes;
}

std::vector<std::string> getTextAxisLabels() {
  return {"f0.Width", "f1.Width", "f2.Width", "f0.EISF", "f1.EISF", "f2.EISF"};
}

std::unique_ptr<QLabel> createLabel(QString const &text) { return std::make_unique<QLabel>(text); }

std::unique_ptr<QComboBox> createComboBox(QStringList const &items) {
  auto combBox = std::make_unique<QComboBox>();
  combBox->addItems(items);
  return combBox;
}

std::unique_ptr<QTableWidget> createEmptyTableWidget(int columns, int rows) {
  auto table = std::make_unique<QTableWidget>(columns, rows);
  for (auto column = 0; column < columns; ++column)
    for (auto row = 0; row < rows; ++row)
      table->setItem(row, column, new QTableWidgetItem("item"));
  return table;
}

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockFqFitDataView : public IIndirectFitDataView {
public:
  /// Public Methods
  MOCK_CONST_METHOD0(getDataTable, QTableWidget *());
  MOCK_METHOD1(validate, UserInputValidator &(UserInputValidator &validator));

  /// Public slots
  MOCK_METHOD1(displayWarning, void(std::string const &warning));
};

class SingleFunctionTemplateBrowserMock : public IFQFitObserver {
  MOCK_METHOD1(updateAvailableFunctions, void(const std::map<std::string, std::string> &functionInitialisationStrings));
};

/// Mock object to mock the model
class MockFqFitModel : public FqFitModel {};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class FqFitDataPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  FqFitDataPresenterTest() { FrameworkManager::Instance(); }

  static FqFitDataPresenterTest *createSuite() { return new FqFitDataPresenterTest(); }

  static void destroySuite(FqFitDataPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockFqFitDataView>>();
    m_model = std::make_unique<NiceMock<MockFqFitModel>>();

    m_dataTable = createEmptyTableWidget(6, 5);
    m_ParameterTypeCombo = createComboBox(getFqFitParameterTypes());
    m_ParameterCombo = createComboBox(getFqFitParameters());
    m_ParameterTypeLabel = createLabel(PARAMETER_TYPE_LABEL);
    m_ParameterLabel = createLabel(PARAMETER_LABEL);
    m_SingleFunctionTemplateBrowser = std::make_unique<SingleFunctionTemplateBrowserMock>();

    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_dataTable.get()));

    m_presenter = std::make_unique<FqFitDataPresenter>(
        std::move(m_model.get()), std::move(m_view.get()), std::move(m_ParameterTypeCombo.get()),
        std::move(m_ParameterCombo.get()), std::move(m_ParameterTypeLabel.get()), std::move(m_ParameterLabel.get()),
        m_SingleFunctionTemplateBrowser.get());

    SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspaceWithTextAxis(6, getTextAxisLabels()));
    m_model->addWorkspace("WorkspaceName");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    m_presenter.reset();
    m_model.reset();
    m_view.reset();

    m_dataTable.reset();
    m_ParameterTypeCombo.reset();
    m_ParameterCombo.reset();
    m_ParameterTypeLabel.reset();
    m_ParameterLabel.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful mock object instantiation
  ///----------------------------------------------------------------------

  void test_that_the_presenter_and_mock_objects_have_been_created() {
    TS_ASSERT(m_presenter);
    TS_ASSERT(m_model);
    TS_ASSERT(m_view);
  }

  void test_that_the_comboboxes_contain_the_items_specified_during_the_setup() {
    auto const parameterTypes = getFqFitParameterTypes();
    auto const parameters = getFqFitParameters();

    TS_ASSERT_EQUALS(m_ParameterTypeCombo->itemText(0), parameterTypes[0]);
    TS_ASSERT_EQUALS(m_ParameterTypeCombo->itemText(1), parameterTypes[1]);
    TS_ASSERT_EQUALS(m_ParameterCombo->itemText(0), parameters[0]);
    TS_ASSERT_EQUALS(m_ParameterCombo->itemText(1), parameters[1]);
  }

  void test_that_the_labels_have_the_correct_text_after_setup() {
    TS_ASSERT_EQUALS(m_ParameterTypeLabel->text(), PARAMETER_TYPE_LABEL);
    TS_ASSERT_EQUALS(m_ParameterLabel->text(), PARAMETER_LABEL);
  }

  void test_that_the_model_contains_the_correct_number_of_workspace_after_instantiation() {
    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), TableDatasetIndex{1});
  }

  /// TODO: Add unittests for setDataIndexToCurrentWorkspace when mainanence
  /// makes tests simpler.

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void test_that_getXRange_calls_the_correct_method_in_the_view() {
    auto const xRange = std::make_pair(0.0, 1.0);

    ON_CALL(*m_view, getXRange()).WillByDefault(Return(xRange));
    EXPECT_CALL(*m_view, getXRange()).Times(1);

    TS_ASSERT_EQUALS(m_presenter->getXRange(), xRange);
  }

private:
  std::unique_ptr<QTableWidget> m_dataTable;
  std::unique_ptr<QComboBox> m_ParameterTypeCombo;
  std::unique_ptr<QComboBox> m_ParameterCombo;
  std::unique_ptr<QLabel> m_ParameterTypeLabel;
  std::unique_ptr<QLabel> m_ParameterLabel;
  std::unique_ptr<SingleFunctionTemplateBrowserMock> m_SingleFunctionTemplateBrowser;

  std::unique_ptr<MockFqFitDataView> m_view;
  std::unique_ptr<MockFqFitModel> m_model;
  std::unique_ptr<FqFitDataPresenter> m_presenter;
};
