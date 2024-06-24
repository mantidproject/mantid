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

  MOCK_METHOD2(checkFileFinderWidgetIsValid, bool(const QString &name, const FileFinderWidget *widget));
  MOCK_METHOD3(checkDataSelectorIsValid, bool(const QString &name, DataSelector *widget, bool silent));

  MOCK_METHOD2(addErrorMessage, void(const std::string &message, bool const silent));

  MOCK_CONST_METHOD0(generateErrorMessage, std::string());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
