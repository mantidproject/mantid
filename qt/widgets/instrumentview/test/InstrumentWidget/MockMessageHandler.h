// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/IMessageHandler.h"

namespace MantidQt::MantidWidgets {

class MockMessageHandler : public IMessageHandler {
public:
  MOCK_METHOD(void, giveUserCritical, (const std::string &, const std::string &), (override));
  MOCK_METHOD(void, giveUserWarning, (const std::string &, const std::string &), (override));
  MOCK_METHOD(void, giveUserInfo, (const std::string &, const std::string &), (override));
  MOCK_METHOD(bool, askUserOkCancel, (const std::string &, const std::string &), (override));
};
} // namespace MantidQt::MantidWidgets
