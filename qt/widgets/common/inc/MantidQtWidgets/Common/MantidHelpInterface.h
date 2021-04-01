// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QWidget>
#include <string>

class QString;
class QUrl;

namespace MantidQt {
namespace API {

/*
 * An interface for handling offline help
 */
class EXPORT_OPT_MANTIDQT_COMMON MantidHelpInterface : public QWidget {
  Q_OBJECT
public:
  /// Default constructor
  MantidHelpInterface();
  /// Default destructor.
  ~MantidHelpInterface() override;

  virtual void showPage(const std::string &url = std::string());
  virtual void showPage(const QString &url);
  virtual void showPage(const QUrl &url);
  virtual void showWikiPage(const std::string &page = std::string());
  virtual void showWikiPage(const QString &page);
  virtual void showAlgorithm(const std::string &name = std::string(), const int version = -1);
  virtual void showAlgorithm(const QString &name, const int version = -1);
  virtual void showConcept(const std::string &name);
  virtual void showConcept(const QString &name);
  virtual void showFitFunction(const std::string &name = std::string());
  virtual void showFitFunction(const QString &name);
  virtual void showCustomInterface(const std::string &name, const std::string &area = std::string(),
                                   const std::string &section = std::string());
  virtual void showCustomInterface(const QString &name, const QString &area = QString(),
                                   const QString &section = QString());

public slots:
  /// Perform any clean up on main window shutdown
  virtual void shutdown();
};
} // namespace API
} // namespace MantidQt
