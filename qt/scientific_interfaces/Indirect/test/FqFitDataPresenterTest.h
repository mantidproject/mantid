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

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockFqFitDataView : public IIndirectFitDataView {
public:
  /// Public Methods
  MOCK_CONST_METHOD0(getDataTable, QTableWidget *());
  MOCK_METHOD1(validate, UserInputValidator &(UserInputValidator &validator));
  MOCK_METHOD2(addTableEntry, void(size_t row, FitDataRow newRow));
  MOCK_CONST_METHOD0(workspaceIndexColumn, int());
  MOCK_CONST_METHOD0(startXColumn, int());
  MOCK_CONST_METHOD0(endXColumn, int());
  MOCK_CONST_METHOD0(excludeColumn, int());
  MOCK_METHOD0(clearTable, void());
  MOCK_CONST_METHOD2(getText, QString(int row, int column));
  MOCK_CONST_METHOD0(getSelectedIndexes, QModelIndexList());
  /// Public slots
  MOCK_METHOD1(displayWarning, void(std::string const &warning));
};

/// Mock object to mock the model
class MockIndirectFitDataModel : public IIndirectFitDataModel {
public:
  /// Public Methods
  MOCK_METHOD0(getFittingData, std::vector<IndirectFitData> *());
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const std::string &spectra));
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const FunctionModelSpectra &spectra));
  MOCK_METHOD2(addWorkspace, void(MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra));
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(FitDomainIndex index));
  MOCK_CONST_METHOD0(getWorkspaceNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(getNumberOfWorkspaces, WorkspaceID());
  MOCK_CONST_METHOD1(hasWorkspace, bool(std::string const &workspaceName));

  MOCK_METHOD2(setSpectra, void(const std::string &spectra, WorkspaceID workspaceID));
  MOCK_METHOD2(setSpectra, void(FunctionModelSpectra &&spectra, WorkspaceID workspaceID));
  MOCK_METHOD2(setSpectra, void(const FunctionModelSpectra &spectra, WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getSpectra, FunctionModelSpectra(WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getSpectrum, size_t(FitDomainIndex index));
  MOCK_CONST_METHOD1(getNumberOfSpectra, size_t(WorkspaceID workspaceID));

  MOCK_METHOD0(clear, void());

  MOCK_CONST_METHOD0(getNumberOfDomains, size_t());
  MOCK_CONST_METHOD2(getDomainIndex, FitDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getSubIndices, std::pair<WorkspaceID, WorkspaceIndex>(FitDomainIndex));

  MOCK_CONST_METHOD0(getQValuesForData, std::vector<double>());
  MOCK_CONST_METHOD0(getResolutionsForFit, std::vector<std::pair<std::string, size_t>>());
  MOCK_CONST_METHOD1(createDisplayName, std::string(WorkspaceID workspaceID));

  MOCK_METHOD1(removeWorkspace, void(WorkspaceID workspaceID));
  MOCK_METHOD1(removeDataByIndex, void(FitDomainIndex fitDomainIndex));

  MOCK_METHOD3(setStartX, void(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setStartX, void(double startX, WorkspaceID workspaceID));
  MOCK_METHOD3(setEndX, void(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setEndX, void(double endX, WorkspaceID workspaceID));
  MOCK_METHOD3(setExcludeRegion, void(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setExcludeRegion, void(const std::string &exclude, FitDomainIndex index));
  MOCK_METHOD1(setResolution, void(const std::string &name));
  MOCK_METHOD2(setResolution, void(const std::string &name, WorkspaceID workspaceID));
  MOCK_CONST_METHOD2(getFittingRange, std::pair<double, double>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getFittingRange, std::pair<double, double>(FitDomainIndex index));
  MOCK_CONST_METHOD2(getExcludeRegion, std::string(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getExcludeRegion, std::string(FitDomainIndex index));
  MOCK_CONST_METHOD2(getExcludeRegionVector, std::vector<double>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getExcludeRegionVector, std::vector<double>(FitDomainIndex index));
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
    m_model = std::make_unique<NiceMock<MockIndirectFitDataModel>>();

    m_dataTable = createEmptyTableWidget(6, 5);
    m_SingleFunctionTemplateBrowser = std::make_unique<SingleFunctionTemplateBrowserMock>();

    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_dataTable.get()));

    m_presenter = std::make_unique<FqFitDataPresenter>(std::move(m_model.get()), std::move(m_view.get()),
                                                       m_SingleFunctionTemplateBrowser.get());
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

  void test_addWorkspace_does_not_throw_with_width() {
    EXPECT_CALL(*m_model, addWorkspace("WorkspaceName_HWHM", FunctionModelSpectra("0"))).Times(Exactly(1));
    m_presenter->addWorkspace("WorkspaceName", "Width", 0);
  }

  void test_addWorkspace_does_not_throw_with_EISF() {
    EXPECT_CALL(*m_model, addWorkspace("WorkspaceName_HWHM", FunctionModelSpectra("3"))).Times(Exactly(1));
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
    TS_ASSERT_THROWS(m_presenter->addWorkspace("WorkspaceName", "InvalidParameter", 0), std::invalid_argument &);
  }

  void test_setActiveWidth_works() {
    ON_CALL(*m_model, getWorkspace(WorkspaceID(0))).WillByDefault(Return(m_workspace));
    m_presenter->setActiveWidth(0, WorkspaceID(0), true);
  }

private:
  std::unique_ptr<QTableWidget> m_dataTable;
  std::unique_ptr<SingleFunctionTemplateBrowserMock> m_SingleFunctionTemplateBrowser;

  std::unique_ptr<MockFqFitDataView> m_view;
  std::unique_ptr<MockIndirectFitDataModel> m_model;
  std::unique_ptr<FqFitDataPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
