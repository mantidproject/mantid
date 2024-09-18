// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "QENSFitting/FitDataView.h"
#include "QENSFitting/FunctionBrowser/SingleFunctionTemplateView.h"
#include "QENSFitting/FunctionQAddWorkspaceDialog.h"
#include "QENSFitting/FunctionQDataPresenter.h"
#include "QENSFitting/FunctionQModel.h"
#include "QENSFitting/InelasticFitPropertyBrowser.h"

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MockObjects.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace testing;

namespace {

QString const PARAMETER_TYPE_LABEL("Fit Parameter:");
QString const PARAMETER_LABEL("Width:");

std::vector<std::string> getTextAxisLabels() {
  return {"f0.Width", "f1.Width", "f2.Width", "f0.EISF", "f1.EISF", "f2.EISF"};
}

std::vector<std::string> getNoAxisLabels() { return {"NoLabel", "NoLabel", "NoLabel"}; }

std::unique_ptr<QTableWidget> createEmptyTableWidget(int columns, int rows) {
  auto table = std::make_unique<QTableWidget>(columns, rows);
  for (auto column = 0; column < columns; ++column)
    for (auto row = 0; row < rows; ++row)
      table->setItem(row, column, new QTableWidgetItem("item"));
  return table;
}

} // namespace

class FunctionQDataPresenterTest : public CxxTest::TestSuite {
public:
  static FunctionQDataPresenterTest *createSuite() { return new FunctionQDataPresenterTest(); }

  static void destroySuite(FunctionQDataPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_tab = std::make_unique<NiceMock<MockFitTab>>();
    m_view = std::make_unique<NiceMock<MockFitDataView>>();
    m_model = std::make_unique<NiceMock<MockDataModel>>();

    m_dataTable = createEmptyTableWidget(6, 5);

    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_dataTable.get()));

    m_presenter = std::make_unique<FunctionQDataPresenter>(m_tab.get(), m_model.get(), m_view.get());
    m_workspace = createWorkspaceWithTextAxis(6, getTextAxisLabels());
    m_ads = std::make_unique<SetUpADSWithWorkspace>("WorkspaceName", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    m_presenter.reset();
    m_model.reset();
    m_view.reset();

    m_dataTable.reset();
  }

  void test_that_the_presenter_and_mock_objects_have_been_created() {
    TS_ASSERT(m_presenter);
    TS_ASSERT(m_model);
    TS_ASSERT(m_view);
  }

  void test_addWorkspaceFromDialog_returns_false_if_the_dialog_is_not_FunctionQ() {
    auto dialog = new MantidQt::MantidWidgets::AddWorkspaceDialog(nullptr);
    TS_ASSERT(!m_presenter->addWorkspaceFromDialog(dialog));
  }

  void test_addWorkspace_does_not_throw_with_width() {
    EXPECT_CALL(*m_model, addWorkspace("WorkspaceName", FunctionModelSpectra("0"))).Times(Exactly(1));
    m_presenter->addWorkspace("WorkspaceName", "Width", 0);
  }

  void test_addWorkspace_does_not_throw_with_EISF() {
    EXPECT_CALL(*m_model, addWorkspace("WorkspaceName", FunctionModelSpectra("3"))).Times(Exactly(1));
    m_presenter->addWorkspace("WorkspaceName", "EISF", 0);
  }

  void test_addWorkspace_throws_with_no_EISF_or_width() {
    auto workspace = createWorkspaceWithTextAxis(3, getNoAxisLabels());
    m_ads->addOrReplace("NoLabelWorkspace", workspace);
    TS_ASSERT_THROWS(m_presenter->addWorkspace("NoLabelWorkspace", "Width", 0), std::invalid_argument &);
  }

  void test_addWorkspace_throws_with_single_bin() {
    auto workspace = createWorkspaceWithTextAxis(6, getTextAxisLabels(), 1);
    m_ads->addOrReplace("singleBinWorkspace", workspace);
    TS_ASSERT_THROWS(m_presenter->addWorkspace("singleBinWorkspace", "Width", 0), std::invalid_argument &);
  }

  void test_addWorkspace_throws_with_invalid_parameter() {
    TS_ASSERT_THROWS(m_presenter->addWorkspace("WorkspaceName", "InvalidParameter", 0), std::logic_error &);
  }

  void test_setActiveSpectra_will_not_error() {
    ON_CALL(*m_model, getWorkspace(WorkspaceID(0))).WillByDefault(Return(m_workspace));
    std::vector<std::size_t> spectra{0u};
    m_presenter->setActiveSpectra(spectra, 0, WorkspaceID(0), true);
  }

private:
  std::unique_ptr<QTableWidget> m_dataTable;

  std::unique_ptr<NiceMock<MockFitTab>> m_tab;
  std::unique_ptr<NiceMock<MockFitDataView>> m_view;
  std::unique_ptr<NiceMock<MockDataModel>> m_model;
  std::unique_ptr<FunctionQDataPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
