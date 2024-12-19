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
#include <QPointer>
#include <QSettings>
#include <QUrl>

using Mantid::Kernel::ConfigService;
using namespace MantidQt::API;

namespace {

namespace ButtonPrefix {
const QString DATA{"pbData"};
const QString SCRIPT{"pbScript"};
const QString EXTENSIONS{"pbExt"};
} // namespace ButtonPrefix

namespace ConfigKeys {
const std::string DATASEARCH_DIRS{"datasearch.directories"};
const std::string PYTHONSCRIPTS_DIRS{"pythonscripts.directories"};
const std::string USERPYTHONPLUGINS_DIRS{"user.python.plugins.directories"};
const std::string DATASEARCH_ARCHIVE{"datasearch.searcharchive"};
const std::string DEFAULT_FACILITY{"default.facility"};
const std::string DEFAULTSAVE_DIR{"defaultsave.directory"};
} // namespace ConfigKeys

namespace QSettingsKeys {
const auto LastDirectory{"ManageUserSettings/last_directory"};
} // namespace QSettingsKeys

// ID for help page in docs
const QString HELP_ID{"ManageUserDirectories"};

// Current instance opened with openManageUserDirectories
QPointer<ManageUserDirectories> CURRENTLY_OPEN_MUD;
} // namespace

/**
 * Show the default dialog or raise the existing one if it exists. It wraps
 * the Mantid::Kernel::ConfigService by default.
 */
ManageUserDirectories *ManageUserDirectories::openManageUserDirectories() {
  if (CURRENTLY_OPEN_MUD.isNull()) {
    CURRENTLY_OPEN_MUD = QPointer<ManageUserDirectories>(new ManageUserDirectories);
    CURRENTLY_OPEN_MUD->show();
  } else {
    CURRENTLY_OPEN_MUD->raise();
  }
  return CURRENTLY_OPEN_MUD;
}

/**
 * Constructor
 * @param parent A parent QWidget for the dialog
 */
ManageUserDirectories::ManageUserDirectories(QWidget *parent) : BaseClass(parent), m_saveToFile(true) {
  setAttribute(Qt::WA_DeleteOnClose);
  m_uiForm.setupUi(this);
  initLayout();
}

/**
 * Control if the config service changes are persisted to the user file.
 * @param enabled If true the config is persisted to the user file
 * after updating the service otherwise only the in-memory store is
 * affected.
 */
void ManageUserDirectories::enableSaveToFile(bool enabled) { m_saveToFile = enabled; }

/**
 * Create the UI layout, fill the widgets and connect the relevant signals
 */
void ManageUserDirectories::initLayout() {
  loadProperties();

  // Make Connections
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(confirmClicked()));

  connect(m_uiForm.pbDataAddDirectory, SIGNAL(clicked()), this, SLOT(addDirectory()));
  connect(m_uiForm.pbScriptAddDirectory, SIGNAL(clicked()), this, SLOT(addDirectory()));
  connect(m_uiForm.pbDataBrowseToDir, SIGNAL(clicked()), this, SLOT(browseToDirectory()));
  connect(m_uiForm.pbScriptBrowseToDir, SIGNAL(clicked()), this, SLOT(browseToDirectory()));
  connect(m_uiForm.pbExtBrowseToDir, SIGNAL(clicked()), this, SLOT(browseToDirectory()));
  connect(m_uiForm.pbDataRemDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbScriptRemDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbExtRemoveDir, SIGNAL(clicked()), this, SLOT(remDir()));
  connect(m_uiForm.pbDataMoveToTop, SIGNAL(clicked()), this, SLOT(moveToTop()));
  connect(m_uiForm.pbDataMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbScriptMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbExtMoveUp, SIGNAL(clicked()), this, SLOT(moveUp()));
  connect(m_uiForm.pbDataMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));
  connect(m_uiForm.pbScriptMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));
  connect(m_uiForm.pbExtMoveDown, SIGNAL(clicked()), this, SLOT(moveDown()));

  connect(m_uiForm.pbSaveBrowse, SIGNAL(clicked()), this, SLOT(selectSaveDir()));
}

/**
 * Load config properties into the form widgets
 */
void ManageUserDirectories::loadProperties() {
  auto &config = ConfigService::Instance();
  auto populateListWidget = [&config](QListWidget *widget, const std::string &key) {
    const auto directories = QString::fromStdString(config.getString(key)).trimmed();
    const auto items = directories.split(";", Qt::SkipEmptyParts);
    widget->clear();
    widget->addItems(items);
  };
  // fill lists
  populateListWidget(m_uiForm.lwDataSearchDirs, ConfigKeys::DATASEARCH_DIRS);
  populateListWidget(m_uiForm.lwScriptSearchDirs, ConfigKeys::PYTHONSCRIPTS_DIRS);
  populateListWidget(m_uiForm.lwExtSearchDirs, ConfigKeys::USERPYTHONPLUGINS_DIRS);

  // set flag of whether to search the data archive
  const auto archive = QString::fromStdString(config.getString(ConfigKeys::DATASEARCH_ARCHIVE)).trimmed().toLower();
  const auto defaultFacility =
      QString::fromStdString(config.getString(ConfigKeys::DEFAULT_FACILITY)).trimmed().toUpper();
  m_uiForm.cbSearchArchive->addItem(QString("default facility only - ") + defaultFacility);
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
  const auto saveDir = QString::fromStdString(config.getString(ConfigKeys::DEFAULTSAVE_DIR)).trimmed();
  m_uiForm.leDefaultSave->setText(saveDir);
}

/**
 *  Save the current contents of the widgets back to the main config
 */
void ManageUserDirectories::saveProperties() {
  auto &config = ConfigService::Instance();

  QString newSearchArchive = m_uiForm.cbSearchArchive->currentText().toLower();
  if (newSearchArchive == "all" || newSearchArchive == "off") {
    // do nothing
  } else if (newSearchArchive.startsWith("default facility only")) {
    newSearchArchive = "on";
  } else {
    // the only way "custom" gets set is by using the value in ConfigService
    // already, so just copy it
    newSearchArchive =
        QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.searcharchive"))
            .trimmed()
            .toLower();
  }

  // convert a path from the list to something the configservice will
  // understand. replaces all \ with / and
  auto toConfigPath = [](QString path) {
    if (path.isEmpty())
      return path;
    auto unixStylePath = path.replace('\\', '/');
    if (!unixStylePath.endsWith('/'))
      unixStylePath.append('/');
    return unixStylePath;
  };
  auto toConfigString = [&toConfigPath](QListWidget *itemsWidget) {
    QStringList paths;
    for (int i = 0; i < itemsWidget->count(); i++) {
      const auto path = itemsWidget->item(i)->text().trimmed();
      if (!path.isEmpty())
        paths.append(toConfigPath(path));
    }
    return paths.join(";").toStdString();
  };

  // data directories
  config.setString(ConfigKeys::DATASEARCH_ARCHIVE, newSearchArchive.toStdString());
  config.setString(ConfigKeys::DATASEARCH_DIRS, toConfigString(m_uiForm.lwDataSearchDirs));
  config.setString(ConfigKeys::DEFAULTSAVE_DIR, toConfigPath(m_uiForm.leDefaultSave->text().trimmed()).toStdString());
  // python directories
  config.setString(ConfigKeys::PYTHONSCRIPTS_DIRS, toConfigString(m_uiForm.lwScriptSearchDirs));
  config.setString(ConfigKeys::USERPYTHONPLUGINS_DIRS, toConfigString(m_uiForm.lwExtSearchDirs));

  if (m_saveToFile)
    config.saveConfig(config.getUserFilename());
}

/**
 * Return the QListWidget related to the given sender
 * @param sender A pointer to the QObject that caused this slot to be called
 * @return QListWidget if sender is a known button else nullptr
 */
QListWidget *ManageUserDirectories::listWidget(QObject *sender) {
  const auto objectName = sender->objectName();
  if (objectName.contains(ButtonPrefix::DATA)) {
    return m_uiForm.lwDataSearchDirs;
  } else if (objectName.contains(ButtonPrefix::SCRIPT)) {
    return m_uiForm.lwScriptSearchDirs;
  } else if (objectName.contains(ButtonPrefix::EXTENSIONS)) {
    return m_uiForm.lwExtSearchDirs;
  } else {
    return nullptr;
  }
}

/// Show the help for ManageUserDirectories
void ManageUserDirectories::helpClicked() { HelpWindow::showCustomInterface(HELP_ID, "framework"); }

/// Close the dialog without saving the configuration
void ManageUserDirectories::cancelClicked() { this->close(); }

/// Persist the properties to the config store and close the dialog
void ManageUserDirectories::confirmClicked() {
  saveProperties();
  this->close();
}

/**
 * Handle the add directory button to take the text from a text
 * box based on the signal sender to the correct list
 */
void ManageUserDirectories::addDirectory() {
  QLineEdit *input(nullptr);

  if (m_uiForm.tabWidget->currentWidget() == m_uiForm.tabDataSearch) {
    input = m_uiForm.leDirectoryPath;
  } else if (m_uiForm.tabWidget->currentWidget() == m_uiForm.tabPythonDirectories) {
    input = m_uiForm.leDirectoryPathPython;
  }

  if (input && input->text() != "") {
    listWidget(sender())->addItem(input->text());
    input->clear();
  }
}

/// Browse to find a new directory. The start directory
/// is the last directory accessed by the application. The
/// directory is added to the list based on the object sender
void ManageUserDirectories::browseToDirectory() {
  QSettings settings;
  const auto lastDirectory = settings.value(QSettingsKeys::LastDirectory, "").toString();

  const auto newDir = QFileDialog::getExistingDirectory(this, tr("Select New Data Directory"), lastDirectory,
                                                        QFileDialog::ShowDirsOnly);

  if (!newDir.isEmpty()) {
    settings.setValue(QSettingsKeys::LastDirectory, newDir);
    listWidget(sender())->addItem(newDir);
  }
}

/// Remove a directory from the list based on the sender
void ManageUserDirectories::remDir() {
  QList<QListWidgetItem *> selected = listWidget(sender())->selectedItems();
  for (auto &i : selected) {
    delete i;
  }
}

/// Move an item in the list based on the sender of the signal and a specified direction
void ManageUserDirectories::moveItem(bool toTop, int offset) {
  QListWidget *list = listWidget(sender());
  QList<QListWidgetItem *> selected = list->selectedItems();
  for (auto &i : selected) {
    int index = list->row(i);
    int count = list->count();
    int newIndex = 0;
    if (toTop) {
      newIndex = 0;
    } else {
      newIndex = index + offset;
    }
    if (newIndex >= 0 && newIndex < count) {
      QListWidgetItem *move = list->takeItem(index);
      list->insertItem(newIndex, move);
    }
    list->setCurrentItem(i);
  }
}

/// Raise an item to the top in the list based on the sender of the signal
void ManageUserDirectories::moveToTop() { moveItem(true, 0); }

/// Raise an item up in the list based on the sender of the signal
void ManageUserDirectories::moveUp() { moveItem(false, -1); }

/// Lower an item down in the list based on the sender of the signal
void ManageUserDirectories::moveDown() { moveItem(false, 1); }

/// Find an existing directory to be used for the save directory path
void ManageUserDirectories::selectSaveDir() {
  QSettings settings;
  auto lastDirectory = m_uiForm.leDefaultSave->text().trimmed();
  if (lastDirectory.isEmpty())
    lastDirectory = settings.value(QSettingsKeys::LastDirectory, "").toString();

  const auto newDir = QFileDialog::getExistingDirectory(this, tr("Select New Default Save Directory"), lastDirectory,
                                                        QFileDialog::ShowDirsOnly);

  if (!newDir.isEmpty()) {
    const auto path = newDir + QDir::separator();
    settings.setValue(QSettingsKeys::LastDirectory, path);
    m_uiForm.leDefaultSave->setText(path);
  }
}
