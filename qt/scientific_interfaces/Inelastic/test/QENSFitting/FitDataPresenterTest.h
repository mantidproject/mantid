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

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidKernel/WarningSuppressions.h"

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
  return [](const std::vector<double> &x, const std::vector<double> &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    // Find data thats within range
    double xmin = range.first;
    double xmax = range.second;

    // If the two points are equal return empty data
    if (fabs(xmin - xmax) < 1e-7) {
      return DataForParameterEstimation{};
    }

    const auto startItr =
        std::find_if(x.cbegin(), x.cend(), [xmin](const double &val) -> bool { return val >= (xmin - 1e-7); });
    auto endItr = std::find_if(x.cbegin(), x.cend(), [xmax](const double &val) -> bool { return val > xmax; });

    if (std::distance(startItr, endItr - 1) < 2)
      return DataForParameterEstimation{};

    size_t first = std::distance(x.cbegin(), startItr);
    size_t end = std::distance(x.cbegin(), endItr);
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
    m_presenter = std::make_unique<FitDataPresenter>(m_tab.get(), m_model.get(), m_view.get());
    m_workspace = createWorkspace(5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("WorkspaceName", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

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
    EXPECT_CALL(*m_model, addWorkspace("WorkspaceName", workpaceIndices)).Times(Exactly(1));
    m_presenter->addWorkspace("WorkspaceName", workpaceIndices);
  }

  void test_setResolution_calls_to_model() {
    ON_CALL(*m_model, setResolution("WorkspaceName")).WillByDefault(Return(true));
    EXPECT_CALL(*m_model, setResolution("WorkspaceName")).Times(Exactly(1));
    m_presenter->setResolution("WorkspaceName");
    EXPECT_CALL(*m_model, removeSpecialValues("WorkspaceName")).Times(Exactly(0));
    EXPECT_CALL(*m_view, displayWarning("Replaced the NaN's and infinities in WorkspaceName with zeros"))
        .Times(Exactly(0));
  }

  void test_setResolution_has_bad_values() {
    ON_CALL(*m_model, setResolution("WorkspaceName")).WillByDefault(Return(false));
    EXPECT_CALL(*m_model, setResolution("WorkspaceName")).Times(Exactly(1));
    EXPECT_CALL(*m_model, removeSpecialValues("WorkspaceName")).Times(Exactly(1));
    EXPECT_CALL(*m_view, displayWarning("Replaced the NaN's and infinities in WorkspaceName with zeros"))
        .Times(Exactly(1));

    m_presenter->setResolution("WorkspaceName");
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

private:
  void deleteSetup() {
    m_presenter.reset();
    m_model.reset();
    m_view.reset();

    m_table.reset();
  }

  void assertValueIsGlobal(int column, TableItem const &value) const {
    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(value.asString(), getTableItem(row, column));
  }

  std::string getTableItem(int row, int column) const { return m_table->item(row, column)->text().toStdString(); }

  std::unique_ptr<QTableWidget> m_table;

  std::unique_ptr<NiceMock<MockFitTab>> m_tab;
  std::unique_ptr<NiceMock<MockFitDataView>> m_view;
  std::unique_ptr<NiceMock<MockDataModel>> m_model;
  std::unique_ptr<FitDataPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
