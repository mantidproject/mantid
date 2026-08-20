// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Spectroscopy/DataModel.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "QENSFitting/FitDataPresenter.h"
#include "QENSFitting/FitDataView.h"
#include "QENSFitting/FittingModel.h"
#include "QENSFitting/IFitDataView.h"
#include "QENSFitting/ParameterEstimation.h"

#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MockObjects.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidKernel/WarningSuppressions.h"
#include <span>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace testing;

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
  TableItem(double const &value) : m_str(QString::number(value, 'g', 16).toStdString()), m_dbl(value) {}

  std::string const &asString() const { return m_str; }
  QString asQString() const { return QString::fromStdString(m_str); }
  double const &asDouble() const { return m_dbl; }

  bool operator==(std::string const &value) const { return this->asString() == value; }

private:
  std::string m_str;
  double m_dbl;
};

class MockDialog : public IAddWorkspaceDialog {

public:
  virtual std::string workspaceName() const override { return "Name"; }
  virtual void setWSSuffices(const QStringList &suffices) override { (void)suffices; }
  virtual void setFBSuffices(const QStringList &suffices) override { (void)suffices; }
  virtual void setLoadProperty(const std::string &propName, bool enabled) override {
    (void)propName;
    (void)enabled;
  }

  virtual void updateSelectedSpectra() override {}
};

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

MATCHER_P(NoCheck, selector, "") { return arg != selector; }

EstimationDataSelector getEstimationDataSelector() {
  return [](std::span<double const> const x, std::span<double const> const y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    // Find data thats within range
    double xmin = range.first;
    double xmax = range.second;

    // If the two points are equal return empty data
    if (fabs(xmin - xmax) < 1e-7) {
      return DataForParameterEstimation{};
    }

    const auto startItr =
        std::find_if(x.begin(), x.end(), [xmin](const double &val) -> bool { return val >= (xmin - 1e-7); });
    auto endItr = std::find_if(x.begin(), x.end(), [xmax](const double &val) -> bool { return val > xmax; });

    if (std::distance(startItr, endItr - 1) < 2)
      return DataForParameterEstimation{};

    size_t first = std::distance(x.begin(), startItr);
    size_t end = std::distance(x.begin(), endItr);
    size_t m = first + (end - first) / 2;

    return DataForParameterEstimation{{x[first], x[m]}, {y[first], y[m]}};
  };
}

GNU_DIAG_ON_SUGGEST_OVERRIDE

class FitDataPresenterTest : public CxxTest::TestSuite {
public:
  static FitDataPresenterTest *createSuite() { return new FitDataPresenterTest(); }

  static void destroySuite(FitDataPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_tab = std::make_unique<NiceMock<MockFitTab>>();
    m_view = std::make_unique<NiceMock<MockFitDataView>>();
    m_model = std::make_unique<NiceMock<MockDataModel>>();
    m_table = createEmptyTableWidget(5, 5);
    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_table.get()));
    m_presenter = std::make_unique<TestableFitDataPresenter>(m_tab.get(), m_model.get(), m_view.get());
    m_workspace = createWorkspace(5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("TestWs", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_tab.get()));

    deleteSetup();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void test_addWorkspaceFromDialog_returns_false_if_the_dialog_is_not_indirect() {
    auto dialog = new MockDialog();
    TS_ASSERT(!m_presenter->addWorkspaceFromDialog(dialog));
  }

  void test_addWorkspaceFromDialog_returns_true_for_a_valid_dialog() {
    auto dialog = new MantidQt::MantidWidgets::AddWorkspaceDialog(nullptr);
    TS_ASSERT(m_presenter->addWorkspaceFromDialog(dialog));
  }

  void test_addWorkspace_with_spectra_calls_to_model() {
    auto workpaceIndices = FunctionModelSpectra("0-3");
    EXPECT_CALL(*m_model, addWorkspace("TestWs", workpaceIndices)).Times(Exactly(1));
    m_presenter->addWorkspace("TestWs", workpaceIndices);
  }

  void test_getResolutionsForFit_calls_from_model() {
    std::vector<std::pair<std::string, size_t>> resolutions = {{"string", 1}};
    EXPECT_CALL(*m_model, getResolutionsForFit()).Times(Exactly(1)).WillOnce(Return(resolutions));
    TS_ASSERT_EQUALS(m_presenter->getResolutionsForFit(), resolutions)
  }

  void test_updateTableFromModel_clears_table_and_adds_new_row_for_each_entry() {
    EXPECT_CALL(*m_view, clearTable()).Times(Exactly(1));
    EXPECT_CALL(*m_model, getNumberOfDomains()).Times(Exactly(4)).WillRepeatedly(Return(3));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(0))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(1))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(2))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_model, updateWorkspaceNames()).Times(Exactly(1));
    FitDataRow newRow;
    EXPECT_CALL(*m_view, addTableEntry(0, _)).Times(Exactly(1));
    EXPECT_CALL(*m_view, addTableEntry(1, _)).Times(Exactly(1));
    EXPECT_CALL(*m_view, addTableEntry(2, _)).Times(Exactly(1));

    m_presenter->updateTableFromModel();
  }

  void test_getNumberOfDomains_calls_from_model() {
    size_t noDomains = 1;
    EXPECT_CALL(*m_model, getNumberOfDomains()).Times(Exactly(1)).WillOnce(Return(noDomains));
    TS_ASSERT_EQUALS(m_presenter->getNumberOfDomains(), noDomains)
  }

  void test_getQValuesForData_calls_from_model() {
    std::vector<double> qValues = {1.0, 2.0, 2.5, -1.5};
    EXPECT_CALL(*m_model, getQValuesForData()).Times(Exactly(1)).WillOnce(Return(qValues));
    TS_ASSERT_EQUALS(m_presenter->getQValuesForData(), qValues)
  }

  ///----------------------------------------------------------------------
  /// Unit Tests for the "Add Numeric Workspace" feature
  ///----------------------------------------------------------------------

  void test_handleAddNumericData_adds_the_workspace_updates_the_table_and_refreshes_the_plot() {
    auto dialog = std::make_unique<MantidQt::MantidWidgets::AddWorkspaceDialog>(nullptr);

    InSequence seq;
    EXPECT_CALL(*m_model, addWorkspace(An<const std::string &>(), _)).Times(Exactly(1));
    EXPECT_CALL(*m_view, clearTable()).Times(Exactly(1));
    EXPECT_CALL(*m_tab, handleNumericDataAdded()).Times(Exactly(1));
    // handleDataChanged is what makes the plot pick up the new data. Without it
    // the row appears in the table but no spectra are plotted.
    EXPECT_CALL(*m_tab, handleDataChanged()).Times(Exactly(1));

    m_presenter->handleAddNumericData(dialog.get());
  }

  void test_handleAddNumericData_does_nothing_if_the_dialog_is_not_an_add_workspace_dialog() {
    auto dialog = new MockDialog();

    EXPECT_CALL(*m_model, addWorkspace(An<const std::string &>(), _)).Times(Exactly(0));
    EXPECT_CALL(*m_tab, handleNumericDataAdded()).Times(Exactly(0));
    EXPECT_CALL(*m_tab, handleDataChanged()).Times(Exactly(0));

    m_presenter->handleAddNumericData(dialog);
  }

  void test_setNumericQAxis_does_nothing_when_given_an_empty_workspace_name() {
    TS_ASSERT_THROWS_NOTHING(m_presenter->setNumericQAxis(""));
  }

  void test_setNumericQAxis_converts_non_numeric_axis_to_numeric_with_momentum_transfer_unit() {
    auto ws = createWorkspace(5);
    TS_ASSERT(!ws->getAxis(1)->isNumeric());

    std::vector<double> originalValues;
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      originalValues.push_back(ws->getAxis(1)->getValue(i));
    }
    m_ads->addOrReplace("NumericWs", ws);

    m_presenter->setNumericQAxis("NumericWs");

    auto *axis = ws->getAxis(1);
    TS_ASSERT(axis->isNumeric());
    TS_ASSERT_EQUALS(axis->unit()->unitID(), "MomentumTransfer");
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(axis->getValue(i), originalValues[i]);
    }
  }

  void test_setNumericQAxis_sets_unit_when_axis_is_already_numeric_with_wrong_unit() {
    auto ws = createWorkspace(5);
    std::vector<double> const qValues{0.1, 0.2, 0.3, 0.4, 0.5};
    ws->replaceAxis(1, std::make_unique<NumericAxis>(qValues));
    TS_ASSERT(ws->getAxis(1)->isNumeric());
    TS_ASSERT_DIFFERS(ws->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    m_ads->addOrReplace("NumericWs", ws);

    m_presenter->setNumericQAxis("NumericWs");

    auto *axis = ws->getAxis(1);
    TS_ASSERT(axis->isNumeric());
    TS_ASSERT_EQUALS(axis->unit()->unitID(), "MomentumTransfer");
    for (size_t i = 0; i < qValues.size(); ++i) {
      TS_ASSERT_EQUALS(axis->getValue(i), qValues[i]);
    }
  }

  void test_setNumericQAxis_leaves_axis_unchanged_when_already_numeric_with_momentum_transfer_unit() {
    auto ws = createWorkspace(5);
    std::vector<double> const qValues{0.1, 0.2, 0.3, 0.4, 0.5};
    ws->replaceAxis(1, std::make_unique<NumericAxis>(qValues));
    ws->getAxis(1)->setUnit("MomentumTransfer");
    m_ads->addOrReplace("NumericWs", ws);

    m_presenter->setNumericQAxis("NumericWs");

    auto *axis = ws->getAxis(1);
    TS_ASSERT(axis->isNumeric());
    TS_ASSERT_EQUALS(axis->unit()->unitID(), "MomentumTransfer");
    for (size_t i = 0; i < qValues.size(); ++i) {
      TS_ASSERT_EQUALS(axis->getValue(i), qValues[i]);
    }
  }

private:
  void deleteSetup() {
    m_presenter.reset();
    m_model.reset();
    m_view.reset();
    m_tab.reset();

    m_table.reset();
  }

  void assertValueIsGlobal(int column, TableItem const &value) const {
    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(value.asString(), getTableItem(row, column));
  }

  std::string getTableItem(int row, int column) const { return m_table->item(row, column)->text().toStdString(); }

  // Exposes the protected setNumericQAxis method so that the axis/unit
  // conversion logic behind the "Add Numeric Workspace" button can be tested
  // directly, without needing to drive the real Qt dialog widgets.
  class TestableFitDataPresenter : public FitDataPresenter {
  public:
    using FitDataPresenter::FitDataPresenter;
    using FitDataPresenter::setNumericQAxis;
  };

  std::unique_ptr<QTableWidget> m_table;

  std::unique_ptr<NiceMock<MockFitTab>> m_tab;
  std::unique_ptr<NiceMock<MockFitDataView>> m_view;
  std::unique_ptr<NiceMock<MockDataModel>> m_model;
  std::unique_ptr<TestableFitDataPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
