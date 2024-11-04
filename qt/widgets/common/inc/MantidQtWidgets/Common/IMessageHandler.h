// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace MantidQt::MantidWidgets {

class IMessageHandler {
public:
  virtual ~IMessageHandler() = default;
  virtual void giveUserCritical(const std::string &prompt, const std::string &title) = 0;
  virtual void giveUserWarning(const std::string &prompt, const std::string &title) = 0;
  virtual void giveUserInfo(const std::string &prompt, const std::string &title) = 0;
  virtual bool askUserOkCancel(const std::string &prompt, const std::string &title) = 0;
};
} // namespace MantidQt::MantidWidgets
