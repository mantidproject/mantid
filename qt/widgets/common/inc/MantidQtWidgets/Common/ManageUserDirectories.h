// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANAGE_USER_DIRECTORIES_H
#define MANTIDQT_MANAGE_USER_DIRECTORIES_H

#include "DllOption.h"
#include "ui_ManageUserDirectories.h"
#include <QDialog>

namespace MantidQt {
namespace API {

class EXPORT_OPT_MANTIDQT_COMMON ManageUserDirectories : public QDialog {
  Q_OBJECT

public:
  ManageUserDirectories(QWidget *parent = nullptr);
  ~ManageUserDirectories() override;
  static void openUserDirsDialog(QWidget *parent);
  void setHelpButtonVisible(const bool &visible);

private:
  virtual void initLayout();
  void loadProperties();
  void saveProperties();
  void appendSlashIfNone(QString &path) const;
  QListWidget *listWidget();

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
  QString m_userPropFile;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_MANAGE_USER_DIRECTORIES_H */
