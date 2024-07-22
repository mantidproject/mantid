// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

using namespace DataValidationHelper;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::MantidWidgets;
using namespace testing;

namespace {

auto constexpr WORKSPACE_NAME = "WorkspaceName";
auto constexpr ERROR_LABEL = "Sample";

std::string const ERROR_MESSAGE_START = "Please correct the following:\n";

std::string workspaceTypeError(std::string const &errorLabel, std::string const &validType) {
  return ERROR_MESSAGE_START + "The " + errorLabel + " workspace is not a " + validType + ".";
}

std::string emptyWorkspaceGroupError() {
  return ERROR_MESSAGE_START + "The group workspace " + WORKSPACE_NAME + " is empty.";
}

MatrixWorkspace_sptr convertWorkspace2DToMatrix(const Workspace2D_sptr &workspace) {
  return std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

MatrixWorkspace_sptr createMatrixWorkspace(std::size_t const &numberOfHistograms, std::size_t const &numberOfBins) {
  return convertWorkspace2DToMatrix(WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBins));
}

TableWorkspace_sptr createTableWorkspace(std::size_t const &size) { return std::make_shared<TableWorkspace>(size); }

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the data selector
class MockDataSelector : public DataSelector {
public:
  /// Public Methods
  MOCK_CONST_METHOD0(getCurrentDataName, QString());
  MOCK_METHOD0(isValid, bool());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class DataValidationHelperTest : public CxxTest::TestSuite {
public:
  DataValidationHelperTest() : m_ads(AnalysisDataService::Instance()) { m_ads.clear(); }

  static DataValidationHelperTest *createSuite() { return new DataValidationHelperTest(); }

  static void destroySuite(DataValidationHelperTest *suite) { delete suite; }

  void setUp() override {
    m_uiv = std::make_unique<UserInputValidator>();
    m_dataSelector = std::make_unique<NiceMock<MockDataSelector>>();
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_dataSelector.get()));

    m_ads.clear();
    m_uiv.reset();
  }

  /// The validateDataIsOfType function only checks the DataSelector against one
  /// data type, so 'isValid' should only be called once
  void test_that_validateDataIsOfType_will_only_call_the_isValid_method_once() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertTheDataIsCheckedOneTime(validateDataIsOfType, DataType::Sqw);
  }

  void
  test_that_validateDataIsOneOf_will_call_the_isValid_method_once_if_the_data_matches_with_a_non_primary_data_types() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertTheDataIsCheckedNTimes(validateDataIsOneOf, 1, DataType::Red, {DataType::Sqw});
  }

  void
  test_that_validateDataIsOneOf_will_call_the_isValid_method_twice_if_the_data_does_not_match_with_the_non_primary_data_type() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertTheDataIsCheckedNTimes(validateDataIsOneOf, 2, DataType::Red, {DataType::Corrections});
  }

  void
  test_that_validateDataIsOneOf_will_call_the_isValid_method_three_times_if_all_three_data_types_do_not_match_the_provided_data() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));
    assertTheDataIsCheckedNTimes(validateDataIsOneOf, 3, DataType::Red, {DataType::Sqw, DataType::Calib});
  }

  void test_that_validateDataIsAReducedFile_will_pass_if_the_workspace_is_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertThatTheDataIsValid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsAReducedFile);
  }

  void test_that_validateDataIsAReducedFile_will_fail_if_the_workspace_is_a_not_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));
    assertThatTheDataIsInvalid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsAReducedFile);
  }

  void
  test_that_validateDataIsAReducedFile_will_return_the_correct_error_message_if_the_workspace_is_not_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));
    assertErrorMessage(WORKSPACE_NAME, ERROR_LABEL, validateDataIsAReducedFile,
                       workspaceTypeError(ERROR_LABEL, "MatrixWorkspace"));
  }

  void test_that_validateDataIsASqwFile_will_pass_if_the_workspace_is_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertThatTheDataIsValid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsASqwFile);
  }

  void test_that_validateDataIsASqwFile_will_fail_if_the_workspace_is_not_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));
    assertThatTheDataIsInvalid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsASqwFile);
  }

  void
  test_that_validateDataIsASqwFile_will_return_the_correct_error_message_if_the_workspace_is_not_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));
    assertErrorMessage(WORKSPACE_NAME, ERROR_LABEL, validateDataIsASqwFile,
                       workspaceTypeError(ERROR_LABEL, "MatrixWorkspace"));
  }

  void test_that_validateDataIsACalibrationFile_will_pass_if_the_workspace_is_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertThatTheDataIsValid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACalibrationFile);
  }

  void test_that_validateDataIsACalibrationFile_will_fail_if_the_workspace_is_not_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));
    assertThatTheDataIsInvalid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACalibrationFile);
  }

  void
  test_that_validateDataIsACalibrationFile_will_return_the_correct_error_message_if_the_workspace_is_not_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));
    assertErrorMessage(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACalibrationFile,
                       workspaceTypeError(ERROR_LABEL, "MatrixWorkspace"));
  }

  void test_that_validateDataIsACorrectionsFile_will_pass_if_the_workspace_is_a_group_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, WorkspaceCreationHelper::createWorkspaceGroup(2, 5, 5, "stem"));
    assertThatTheDataIsValid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACorrectionsFile);
  }

  void test_that_validateDataIsACorrectionsFile_will_fail_if_the_workspace_is_not_a_group_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertThatTheDataIsInvalid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACorrectionsFile);
  }

  void
  test_that_validateDataIsACorrectionsFile_will_return_the_correct_error_message_if_the_workspace_is_not_a_group_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    assertErrorMessage(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACorrectionsFile,
                       workspaceTypeError(ERROR_LABEL, "WorkspaceGroup"));
  }

  void test_that_validateDataIsACorrectionsFile_will_fail_if_the_workspace_group_is_empty() {
    m_ads.addOrReplace(WORKSPACE_NAME, std::make_shared<WorkspaceGroup>());
    assertThatTheDataIsInvalid(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACorrectionsFile);
  }

  void
  test_that_validateDataIsACorrectionsFile_will_return_the_correct_error_message_if_the_workspace_group_is_empty() {
    m_ads.addOrReplace(WORKSPACE_NAME, std::make_shared<WorkspaceGroup>());
    assertErrorMessage(WORKSPACE_NAME, ERROR_LABEL, validateDataIsACorrectionsFile, emptyWorkspaceGroupError());
  }

private:
  template <typename Functor> void assertTheDataIsCheckedOneTime(Functor const &functor, DataType const &primaryType) {
    ON_CALL(*m_dataSelector, getCurrentDataName()).WillByDefault(Return(QString::fromStdString(WORKSPACE_NAME)));
    ON_CALL(*m_dataSelector, isValid()).WillByDefault(Return(true));

    EXPECT_CALL(*m_dataSelector, getCurrentDataName()).Times(1);
    EXPECT_CALL(*m_dataSelector, isValid()).Times(1);

    (void)functor(m_uiv.get(), m_dataSelector.get(), ERROR_LABEL, primaryType, false);
  }

  template <typename Functor>
  void assertTheDataIsCheckedNTimes(Functor const &functor, int nTimes, DataType const &primaryType,
                                    std::vector<DataType> const &otherTypes) {
    ON_CALL(*m_dataSelector, getCurrentDataName()).WillByDefault(Return(QString::fromStdString(WORKSPACE_NAME)));
    ON_CALL(*m_dataSelector, isValid()).WillByDefault(Return(true));

    EXPECT_CALL(*m_dataSelector, getCurrentDataName()).Times(nTimes);
    EXPECT_CALL(*m_dataSelector, isValid()).Times(nTimes);

    (void)functor(m_uiv.get(), m_dataSelector.get(), ERROR_LABEL, primaryType, otherTypes, false);
  }

  template <typename Functor>
  void assertThatTheDataIsValid(std::string const &workspaceName, std::string const &errorLabel,
                                Functor const &functor) {
    ON_CALL(*m_dataSelector, getCurrentDataName()).WillByDefault(Return(QString::fromStdString(workspaceName)));
    ON_CALL(*m_dataSelector, isValid()).WillByDefault(Return(true));

    TS_ASSERT(functor(m_uiv.get(), m_dataSelector.get(), errorLabel, false));
    TS_ASSERT(m_uiv->generateErrorMessage().empty());
  }

  template <typename Functor>
  void assertThatTheDataIsInvalid(std::string const &workspaceName, std::string const &errorLabel,
                                  Functor const &functor) {
    ON_CALL(*m_dataSelector, getCurrentDataName()).WillByDefault(Return(QString::fromStdString(workspaceName)));
    ON_CALL(*m_dataSelector, isValid()).WillByDefault(Return(true));

    TS_ASSERT(!functor(m_uiv.get(), m_dataSelector.get(), errorLabel, false));
    TS_ASSERT(!m_uiv->generateErrorMessage().empty());
  }

  template <typename Functor>
  void assertErrorMessage(std::string const &workspaceName, std::string const &errorLabel, Functor const &functor,
                          std::string const &errorMessage) {
    ON_CALL(*m_dataSelector, getCurrentDataName()).WillByDefault(Return(QString::fromStdString(workspaceName)));
    ON_CALL(*m_dataSelector, isValid()).WillByDefault(Return(true));

    (void)functor(m_uiv.get(), m_dataSelector.get(), errorLabel, false);

    TS_ASSERT_EQUALS(m_uiv->generateErrorMessage(), errorMessage);
  }

  AnalysisDataServiceImpl &m_ads;
  std::unique_ptr<UserInputValidator> m_uiv;
  std::unique_ptr<MockDataSelector> m_dataSelector;
};
