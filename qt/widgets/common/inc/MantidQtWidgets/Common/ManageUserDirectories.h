// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Common/MantidDialog.h"
#else
#include <QDialog>
#endif
#include "ui_ManageUserDirectories.h"

namespace MantidQt {
namespace API {

/**
 * Access and update the user directory settings within the
 * Mantid config service
 */
class EXPORT_OPT_MANTIDQT_COMMON ManageUserDirectories
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    : public MantidQt::API::MantidDialog {
  using BaseClass = MantidQt::API::MantidDialog;
#else
    : public QDialog {
  using BaseClass = QDialog;
#endif

  Q_OBJECT

public:
  static ManageUserDirectories *openManageUserDirectories();

public:
  ManageUserDirectories(QWidget *parent = nullptr);

  void enableSaveToFile(bool enabled);

public:
  /// Testing accessors
  QPushButton *cancelButton() const { return m_uiForm.pbCancel; }

private:
  virtual void initLayout();
  void loadProperties();
  void saveProperties();
  QListWidget *listWidget(QObject *object);

private slots:
  void helpClicked();
  void cancelClicked();
  void confirmClicked();
  void addDirectory();
  void browseToDirectory();
  void remDir();
  void moveUp();
  void moveDown();
  void selectSaveDir();

private:
  Ui::ManageUserDirectories m_uiForm;
  bool m_saveToFile;
};

} // namespace API
} // namespace MantidQt
