#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include <QString>
#include <QUrl>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class PythonHelpWindow : public MantidQt::API::MantidHelpInterface {
public:
  PythonHelpWindow();
  void showPage(const std::string &url = std::string()) override;
  void showPage(const QString &url) override;
  void showPage(const QUrl &url) override;
  void showAlgorithm(const std::string &name = std::string(), const int version = -1) override;
  void showAlgorithm(const QString &name, const int version = -1) override;
  void showConcept(const std::string &name) override;
  void showConcept(const QString &name) override;
  void showFitFunction(const std::string &name = std::string()) override;
  void showFitFunction(const QString &name) override;
  void showCustomInterface(const std::string &name = std::string(), const std::string &area = std::string(),
                           const std::string &section = std::string()) override;
  void showCustomInterface(const QString &name, const QString &area = QString(),
                           const QString &section = QString()) override;
  void shutdown() override;
};

} // namespace MantidWidgets
} // namespace MantidQt
