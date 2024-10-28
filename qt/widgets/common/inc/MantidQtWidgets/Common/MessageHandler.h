// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "IMessageHandler.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <string>

namespace MantidQt::MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON MessageHandler final : public IMessageHandler {
public:
  ~MessageHandler() = default;
  void giveUserCritical(const std::string &prompt, const std::string &title) override;
  void giveUserWarning(const std::string &prompt, const std::string &title) override;
  void giveUserInfo(const std::string &prompt, const std::string &title) override;
  bool askUserOkCancel(const std::string &prompt, const std::string &title) override;
};
} // namespace MantidQt::MantidWidgets
