// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include <QDir>
#include <QFileDialog>
#include <QSettings>
#include <QUrl>

using namespace MantidQt::API;

namespace {
std::unique_ptr<ManageUserDirectories> CURRENTLY_OPEN_MUD;
} // namespace

namespace ButtonPrefix {
const std::string DATA{"pbData"};
const std::string SCRIPT{"pbScript"};
const std::string EXTENSIONS{"pbExt"};
} // namespace ButtonPrefix

ManageUserDirectories::ManageUserDirectories(QWidget *parent)
    : MantidDialog(parent) {
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

  connect(m_uiForm.pbDataAddDirectory, SIGNAL(clicked()), this,
          SLOT(addDirectory()));
  connect(m_uiForm.pbScriptAddDirectory, SIGNAL(clicked()), this,
          SLOT(addDirectory()));
  connect(m_uiForm.pbDataBrowseToDir, SIGNAL(clicked()), this,
          SLOT(browseToDirectory()));
  connect(m_uiForm.pbScriptBrowseToDir, SIGNAL(clicked()), this,
          SLOT(browseToDirectory()));
  connect(m_uiForm.pbExtBrowseToDir, SIGNAL(clicked()), this,
          SLOT(browseToDirectory()));
  connect(m_uiForm.pbDataRemDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbScriptRemDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbExtRemoveDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbDataMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbScriptMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbExtMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbDataMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));
  connect(m_uiForm.pbScriptMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));
  connect(m_uiForm.pbExtMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));

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
  m_uiForm.lwScriptSearchDirs->clear();
  m_uiForm.lwScriptSearchDirs->addItems(list);

  directories = QString::fromStdString(
                    Mantid::Kernel::ConfigService::Instance().getString(
                        "user.python.plugins.directories"))
                    .trimmed();
  list = directories.split(";", QString::SkipEmptyParts);
  m_uiForm.lwExtSearchDirs->clear();
  m_uiForm.lwExtSearchDirs->addItems(list);

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
  QStringList extensionDirs;

  for (int i = 0; i < m_uiForm.lwDataSearchDirs->count(); i++) {
    QString dir = m_uiForm.lwDataSearchDirs->item(i)->text();
    appendSlashIfNone(dir);
    dataDirs.append(dir);
  }

  for (int i = 0; i < m_uiForm.lwScriptSearchDirs->count(); i++) {
    QString dir = m_uiForm.lwScriptSearchDirs->item(i)->text();
    appendSlashIfNone(dir);
    userDirs.append(dir);
  }

  for (int i = 0; i < m_uiForm.lwExtSearchDirs->count(); i++) {
    QString dir = m_uiForm.lwExtSearchDirs->item(i)->text();
    appendSlashIfNone(dir);
    extensionDirs.append(dir);
  }
  QString newDataDirs;
  QString newUserDirs;
  QString newSaveDir;
  QString newExtensionDirs;

  newDataDirs = dataDirs.join(";");
  newUserDirs = userDirs.join(";");
  newDataDirs.replace('\\', '/');
  newUserDirs.replace('\\', '/');

  // Extensions dirs
  newExtensionDirs = extensionDirs.join(";");
  newExtensionDirs.replace('\\', '/');

  newSaveDir = m_uiForm.leDefaultSave->text();
  newSaveDir.replace('\\', '/');
  appendSlashIfNone(newSaveDir);

  Mantid::Kernel::ConfigServiceImpl &config =
      Mantid::Kernel::ConfigService::Instance();

  config.setString("datasearch.searcharchive", newSearchArchive.toStdString());
  config.setString("datasearch.directories", newDataDirs.toStdString());
  config.setString("defaultsave.directory", newSaveDir.toStdString());
  config.setString("pythonscripts.directories", newUserDirs.toStdString());
  config.setString("user.python.plugins.directories",
                   newExtensionDirs.toStdString());
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

QListWidget *ManageUserDirectories::listWidget(QObject *sender) {
  std::string objectName = sender->objectName().toStdString();
  if (objectName.find(ButtonPrefix::DATA) != std::string::npos) {
    return m_uiForm.lwDataSearchDirs;
  } else if (objectName.find(ButtonPrefix::SCRIPT) != std::string::npos) {
    return m_uiForm.lwScriptSearchDirs;
  } else if (objectName.find(ButtonPrefix::EXTENSIONS) != std::string::npos) {
    return m_uiForm.lwExtSearchDirs;
  } else {
    return nullptr;
  }
}

// SLOTS
void ManageUserDirectories::helpClicked() {
  HelpWindow::showCustomInterface(nullptr, QString("ManageUserDirectories"));
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
    listWidget(sender())->addItem(input->text());
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
    listWidget(sender())->addItem(newDir);
  }
}
void ManageUserDirectories::remDir() {
  QList<QListWidgetItem *> selected = listWidget(sender())->selectedItems();
  for (auto &i : selected) {
    delete i;
  }
}
void ManageUserDirectories::moveUp() {
  QListWidget *list = listWidget(sender());
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
  QListWidget *list = listWidget(sender());
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

void ManageUserDirectories::openManageUserDirectories() {
  if (CURRENTLY_OPEN_MUD) {
    CURRENTLY_OPEN_MUD->raise();
  } else {
    CURRENTLY_OPEN_MUD = std::make_unique<ManageUserDirectories>();
    CURRENTLY_OPEN_MUD->show();
  }
}

void ManageUserDirectories::closeEvent(QCloseEvent *event) {
  CURRENTLY_OPEN_MUD.reset();
  QWidget::closeEvent(event);
}
