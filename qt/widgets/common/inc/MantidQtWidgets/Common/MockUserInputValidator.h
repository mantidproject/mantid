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

  MOCK_CONST_METHOD0(generateErrorMessage, std::string());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
