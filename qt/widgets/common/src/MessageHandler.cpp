// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MessageHandler.h"

#include <QMessageBox>

#include <cassert>
#include <string>

namespace MantidQt::MantidWidgets {

void MessageHandler::giveUserCritical(const std::string &prompt, const std::string &title) {
  QMessageBox::critical(nullptr, QString::fromStdString(title), QString::fromStdString(prompt), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void MessageHandler::giveUserWarning(const std::string &prompt, const std::string &title) {
  QMessageBox::warning(nullptr, QString::fromStdString(title), QString::fromStdString(prompt), QMessageBox::Ok,
                       QMessageBox::Ok);
}
void MessageHandler::giveUserInfo(const std::string &prompt, const std::string &title) {
  QMessageBox::information(nullptr, QString::fromStdString(title), QString::fromStdString(prompt), QMessageBox::Ok,
                           QMessageBox::Ok);
}

bool MessageHandler::askUserOkCancel(const std::string &prompt, const std::string &title) {
  auto reply = QMessageBox::question(nullptr, QString::fromStdString(title), QString::fromStdString(prompt),
                                     QMessageBox::Ok | QMessageBox::Cancel);
  return (reply == QMessageBox::Ok);
}
} // namespace MantidQt::MantidWidgets
