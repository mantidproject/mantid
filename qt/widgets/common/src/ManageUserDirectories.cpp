// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include <QDir>
#include <QFileDialog>
#include <QSettings>
#include <QUrl>

using namespace MantidQt::API;

ManageUserDirectories::ManageUserDirectories(QWidget *parent)
    : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  m_uiForm.setupUi(this);
  initLayout();
}

ManageUserDirectories::~ManageUserDirectories() {}

void ManageUserDirectories::initLayout() {
  loadProperties();

  // Make Connections
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(confirmClicked()));

  connect(m_uiForm.pbAddDirectory, SIGNAL(clicked()), this,
          SLOT(addDirectory()));
  connect(m_uiForm.pbAddDirectoryPython, SIGNAL(clicked()), this,
          SLOT(addDirectory()));
  connect(m_uiForm.pbBrowseToDir, SIGNAL(clicked()), this,
          SLOT(browseToDirectory()));
  connect(m_uiForm.pbBrowseToDirPython, SIGNAL(clicked()), this,
          SLOT(browseToDirectory()));
  connect(m_uiForm.pbRemDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbRemDirPython, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbMoveUpPython, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));
  connect(m_uiForm.pbMoveDownPython, SIGNAL(clicked()), this, SLOT(moveDown()));

  connect(m_uiForm.pbSaveBrowse, SIGNAL(clicked()), this,
          SLOT(selectSaveDir()));
}

void ManageUserDirectories::loadProperties() {
  m_userPropFile =
      QString::fromStdString(
          Mantid::Kernel::ConfigService::Instance().getUserFilename())
          .trimmed();

  // get data search directories and populate the list widget (lwDataSearchDirs)
  QString directories = QString::fromStdString(
                            Mantid::Kernel::ConfigService::Instance().getString(
                                "datasearch.directories"))
                            .trimmed();
  QStringList list = directories.split(";", QString::SkipEmptyParts);
  m_uiForm.lwDataSearchDirs->clear();
  m_uiForm.lwDataSearchDirs->addItems(list);

  // Do the same thing for the "pythonscripts.directories" property.
  directories = QString::fromStdString(
                    Mantid::Kernel::ConfigService::Instance().getString(
                        "pythonscripts.directories"))
                    .trimmed();
  list = directories.split(";", QString::SkipEmptyParts);
  m_uiForm.lwUserSearchDirs->clear();
  m_uiForm.lwUserSearchDirs->addItems(list);

  // set flag of whether to search the data archive
  QString archive = QString::fromStdString(
                        Mantid::Kernel::ConfigService::Instance().getString(
                            "datasearch.searcharchive"))
                        .trimmed()
                        .toLower();
  QString defaultFacility =
      QString::fromStdString(
          Mantid::Kernel::ConfigService::Instance().getString(
              "default.facility"))
          .trimmed()
          .toUpper();
  m_uiForm.cbSearchArchive->addItem(QString("default facility only - ") +
                                    defaultFacility);
  m_uiForm.cbSearchArchive->addItem("all");
  m_uiForm.cbSearchArchive->addItem("off");
  if (archive == "on") {
    m_uiForm.cbSearchArchive->setCurrentIndex(0);
  } else if (archive == "all") {
    m_uiForm.cbSearchArchive->setCurrentIndex(1);
  } else if (archive == "off") {
    m_uiForm.cbSearchArchive->setCurrentIndex(2);
  } else { // only add custom if it has been set
    m_uiForm.cbSearchArchive->addItem("custom - " + archive.toUpper());
    m_uiForm.cbSearchArchive->setCurrentIndex(3);
  }

  // default save directory
  QString saveDir = QString::fromStdString(
                        Mantid::Kernel::ConfigService::Instance().getString(
                            "defaultsave.directory"))
                        .trimmed();
  m_uiForm.leDefaultSave->setText(saveDir);
}
void ManageUserDirectories::saveProperties() {
  QString newSearchArchive = m_uiForm.cbSearchArchive->currentText().toLower();
  if (newSearchArchive == "all" || newSearchArchive == "off") {
    // do nothing
  } else if (newSearchArchive.startsWith("default facility only")) {
    newSearchArchive = "on";
  } else {
    // the only way "custom" gets set is by using the value in ConfigService
    // already, so just copy it
    newSearchArchive = QString::fromStdString(
                           Mantid::Kernel::ConfigService::Instance().getString(
                               "datasearch.searcharchive"))
                           .trimmed()
                           .toLower();
  }

  QStringList dataDirs;
  QStringList userDirs;

  for (int i = 0; i < m_uiForm.lwDataSearchDirs->count(); i++) {
    QString dir = m_uiForm.lwDataSearchDirs->item(i)->text();
    appendSlashIfNone(dir);
    dataDirs.append(dir);
  }

  for (int i = 0; i < m_uiForm.lwUserSearchDirs->count(); i++) {
    QString dir = m_uiForm.lwUserSearchDirs->item(i)->text();
    appendSlashIfNone(dir);
    userDirs.append(dir);
  }

  QString newDataDirs;
  QString newUserDirs;
  QString newSaveDir;

  newDataDirs = dataDirs.join(";");
  newUserDirs = userDirs.join(";");
  newDataDirs.replace('\\', '/');
  newUserDirs.replace('\\', '/');

  newSaveDir = m_uiForm.leDefaultSave->text();
  newSaveDir.replace('\\', '/');
  appendSlashIfNone(newSaveDir);

  Mantid::Kernel::ConfigServiceImpl &config =
      Mantid::Kernel::ConfigService::Instance();

  config.setString("datasearch.searcharchive", newSearchArchive.toStdString());
  config.setString("datasearch.directories", newDataDirs.toStdString());
  config.setString("defaultsave.directory", newSaveDir.toStdString());
  config.setString("pythonscripts.directories", newUserDirs.toStdString());
  config.saveConfig(m_userPropFile.toStdString());
}

/**
 * Appends a forward slash to the end of a path if there is no slash (forward or
 * back) there already, and strip whitespace from the path.
 *
 * @param path :: A reference to the path
 */
void ManageUserDirectories::appendSlashIfNone(QString &path) const {
  path = path.trimmed();
  if (!(path.endsWith("/") || path.endsWith("\\") || path.isEmpty())) {
    // Don't need to add to a \\, as it would just get changed to a /
    // immediately after
    path.append("/");
  }
}

QListWidget *ManageUserDirectories::listWidget() {
  if (m_uiForm.tabWidget->currentWidget() == m_uiForm.tabDataSearch) {
    return m_uiForm.lwDataSearchDirs;
  } else if (m_uiForm.tabWidget->currentWidget() ==
             m_uiForm.tabPythonDirectories) {
    return m_uiForm.lwUserSearchDirs;
  } else {
    return nullptr;
  }
}

// SLOTS
void ManageUserDirectories::helpClicked() {
  HelpWindow::showCustomInterface(this, QString("ManageUserDirectories"));
}
void ManageUserDirectories::cancelClicked() { this->close(); }
void ManageUserDirectories::confirmClicked() {
  saveProperties();
  this->close();
}

void ManageUserDirectories::addDirectory() {
  QLineEdit *input(nullptr);

  if (m_uiForm.tabWidget->currentWidget() == m_uiForm.tabDataSearch) {
    input = m_uiForm.leDirectoryPath;
  } else if (m_uiForm.tabWidget->currentWidget() ==
             m_uiForm.tabPythonDirectories) {
    input = m_uiForm.leDirectoryPathPython;
  }

  if (input && input->text() != "") {
    listWidget()->addItem(input->text());
    input->clear();
  }
}

void ManageUserDirectories::browseToDirectory() {
  QSettings settings;
  QString lastDirectory =
      settings.value("ManageUserSettings/last_directory", "").toString();

  QString newDir = QFileDialog::getExistingDirectory(
      this, tr("Select New Data Directory"), lastDirectory,
      QFileDialog::ShowDirsOnly);

  if (newDir != "") {
    settings.setValue("ManageUserSettings/last_directory", newDir);
    listWidget()->addItem(newDir);
  }
}
void ManageUserDirectories::remDir() {
  QList<QListWidgetItem *> selected = listWidget()->selectedItems();
  for (auto &i : selected) {
    delete i;
  }
}
void ManageUserDirectories::moveUp() {
  QListWidget *list = listWidget();
  QList<QListWidgetItem *> selected = list->selectedItems();
  for (auto &i : selected) {
    int index = list->row(i);
    if (index != 0) {
      QListWidgetItem *move = list->takeItem(index);
      list->insertItem(index - 1, move);
    }
    list->setCurrentItem(i);
  }
}
void ManageUserDirectories::moveDown() {
  QListWidget *list = listWidget();
  int count = list->count();
  QList<QListWidgetItem *> selected = list->selectedItems();
  for (auto &i : selected) {
    int index = list->row(i);
    if (index != (count - 1)) {
      QListWidgetItem *move = list->takeItem(index);
      list->insertItem(index + 1, move);
    }
    list->setCurrentItem(i);
  }
}
void ManageUserDirectories::selectSaveDir() {
  QSettings settings;
  QString lastDirectory = m_uiForm.leDefaultSave->text();
  if (lastDirectory.trimmed() == "")
    lastDirectory =
        settings.value("ManageUserSettings/last_directory", "").toString();

  const QString newDir = QFileDialog::getExistingDirectory(
      this, tr("Select New Default Save Directory"), lastDirectory,
      QFileDialog::ShowDirsOnly);

  if (newDir != "") {
    QString path = newDir + QDir::separator();
    path.replace('\\', '/');
    settings.setValue("ManageUserSettings/last_directory", path);
    m_uiForm.leDefaultSave->setText(path);
  }
}
/** Opens a manage directories dialog and gives it focus
 *  @param parent :: the parent window, probably the window that called it
 */
void ManageUserDirectories::openUserDirsDialog(QWidget *parent) {
  ManageUserDirectories *ad = new ManageUserDirectories(parent);
  ad->show();
  ad->setFocus();
}
