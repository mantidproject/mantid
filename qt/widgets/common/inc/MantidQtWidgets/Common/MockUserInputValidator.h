// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <string>

#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockUserInputValidator : public MantidQt::CustomInterfaces::IUserInputValidator {
public:
  virtual ~MockUserInputValidator() = default;

  MOCK_METHOD3(checkFieldIsNotEmpty, bool(const QString &name, QLineEdit *field, QLabel *errorLabel));
  MOCK_METHOD3(checkFieldIsValid, bool(const QString &errorMessage, QLineEdit *field, QLabel *errorLabel));
  MOCK_METHOD2(checkWorkspaceSelectorIsNotEmpty, bool(const QString &name, WorkspaceSelector *workspaceSelector));
  MOCK_METHOD2(checkFileFinderWidgetIsValid, bool(const QString &name, const FileFinderWidget *widget));
  MOCK_METHOD3(checkDataSelectorIsValid, bool(const QString &name, DataSelector *widget, bool silent));
  MOCK_METHOD3(checkWorkspaceGroupIsValid, bool(QString const &groupName, QString const &inputType, bool silent));
  MOCK_METHOD2(checkWorkspaceExists, bool(QString const &workspaceName, bool silent));
  MOCK_METHOD2(checkValidRange, bool(QString const &name, std::pair<double, double> range));
  MOCK_METHOD2(checkRangesDontOverlap, bool(std::pair<double, double> rangeA, std::pair<double, double> rangeB));
  MOCK_METHOD4(checkRangeIsEnclosed, bool(const QString &outerName, std::pair<double, double> outer,
                                          const QString &innerName, std::pair<double, double> inner));
  MOCK_METHOD4(checkBins, bool(double lower, double binWidth, double upper, double tolerance));

  MOCK_METHOD2(setErrorLabel, void(QLabel *errorLabel, bool valid));

  MOCK_METHOD2(addErrorMessage, void(const std::string &message, bool const silent));

  MOCK_CONST_METHOD0(generateErrorMessage, std::string());
  MOCK_CONST_METHOD0(isAllInputValid, bool());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
