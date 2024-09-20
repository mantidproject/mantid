// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Exception.h"

#include <QFileInfo>

#include <QDebug>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

using namespace Mantid::API;

namespace {
auto &ads = AnalysisDataService::Instance();

std::string cutLastOf(const std::string &str, const std::string &delimiter) {
  const auto cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}

std::string extractLastOf(const std::string &str, const std::string &delimiter) {
  const auto cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(cutIndex + 1, str.size() - cutIndex);
  return str;
}

bool fileFound(std::string const &file) { return !FileFinder::Instance().getFullPath(file).empty(); }

std::string loadAlgName(const std::string &filePath) {
  const auto suffix = extractLastOf(filePath, ".");
  return suffix == "dave" ? "LoadDaveGrp" : "Load";
}

void makeGroup(std::string const &workspaceName) {
  if (!ads.retrieveWS<WorkspaceGroup>(workspaceName)) {
    const auto groupAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setProperty("InputWorkspaces", workspaceName);
    groupAlg->setProperty("OutputWorkspace", workspaceName);
    groupAlg->execute();
  }
}

} // namespace

namespace MantidQt::MantidWidgets {

DataSelector::DataSelector(QWidget *parent)
    : API::MantidWidget(parent), m_loadProperties(), m_algRunner(), m_autoLoad(true), m_showLoad(true),
      m_alwaysLoadAsGroup(false) {
  m_uiForm.setupUi(this);
  connect(m_uiForm.cbInputType, SIGNAL(currentIndexChanged(int)), this, SLOT(handleViewChanged(int)));
  connect(m_uiForm.pbLoadFile, SIGNAL(clicked()), this, SIGNAL(loadClicked()));

  // data selected changes
  connect(m_uiForm.rfFileInput, SIGNAL(filesFoundChanged()), this, SLOT(handleFileInput()));
  connect(m_uiForm.wsWorkspaceInput, SIGNAL(currentIndexChanged(int)), this, SLOT(handleWorkspaceInput()));
  connect(m_uiForm.pbLoadFile, SIGNAL(clicked()), this, SLOT(handleFileInput()));

  connect(&m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(handleAutoLoadComplete(bool)));
  this->setAcceptDrops(true);
  m_uiForm.rfFileInput->setAcceptDrops(false);
}

DataSelector::~DataSelector() = default;

/**
 * Return whether empty input is allowed
 * @return :: Flag if is optional
 */
bool DataSelector::isOptional() const { return m_isOptional; }

/**
 * Sets if the text field is optional
 * @param optional :: Set the optional status of the text field
 */
void DataSelector::isOptional(bool optional) {
  m_isOptional = optional;
  m_uiForm.rfFileInput->isOptional(optional);
  m_uiForm.wsWorkspaceInput->setOptional(optional);
}

/**
 * Handle signals when files are found or the user manually clicks load.
 */
void DataSelector::handleFileInput() {
  // Get filename and check it's not empty
  QString filename = m_uiForm.rfFileInput->getUserInput().toString();

  if (filename.isEmpty()) {
    return;
  }

  // attempt to load the file
  if (m_autoLoad) {
    emit filesAutoLoaded();
    autoLoadFile(filename);
  } else {
    // files were found
    emit filesFound();
  }
}

/**
 * Sets whether the file or workspace selector is visible
 */
void DataSelector::setSelectorIndex(int index) { m_uiForm.cbInputType->setCurrentIndex(index); }

/**
 * Sets whether the file or workspace type selector is visible
 */
void DataSelector::setTypeSelectorVisible(bool visible) { m_uiForm.cbInputType->setVisible(visible); }

/**
 * Get if the file selector is currently being shown.
 *
 * @return :: true if it is visible, otherwise false
 */
bool DataSelector::isFileSelectorVisible() const {
  int index = m_uiForm.stackedDataSelect->currentIndex();
  return (index == 0);
}

/**
 * Get if the workspace selector is currently being shown.
 *
 * @return :: true if it is visible, otherwise false
 */
bool DataSelector::isWorkspaceSelectorVisible() const { return !isFileSelectorVisible(); }

/**
 * Check if the data selector is in a valid state
 *
 * Checks using the relvant widgets isValid method depending
 * on what view is currently being shown
 *
 * @return :: If the data selector is valid
 */
bool DataSelector::isValid() {
  bool isValid = false;

  if (isFileSelectorVisible()) {
    isValid = m_uiForm.rfFileInput->isValid();

    // check to make sure the user hasn't deleted the auto-loaded file
    // since choosing it.
    if (isValid && m_autoLoad) {
      auto const wsName = getCurrentDataName().toStdString();

      isValid = !wsName.empty();
      if (isValid && !ads.doesExist(wsName)) {
        // attempt to reload if we can
        // don't use algorithm runner because we need to know instantly.
        auto const filepath = m_uiForm.rfFileInput->getUserInput().toString().toStdString();
        if (!filepath.empty())
          executeLoadAlgorithm(filepath, wsName);

        isValid = ads.doesExist(wsName);

        if (!isValid) {
          m_uiForm.rfFileInput->setFileProblem("The specified workspace is "
                                               "missing from the analysis data "
                                               "service");
        }
      } else {
        if (!ads.doesExist(wsName)) {
          return isValid;
        }
        auto const workspaceTypes = m_uiForm.wsWorkspaceInput->getWorkspaceTypes();
        auto const workspace = ads.retrieveWS<Workspace>(wsName);
        isValid = workspaceTypes.empty() || workspaceTypes.indexOf(QString::fromStdString(workspace->id())) != -1;
        if (!isValid) {
          m_uiForm.rfFileInput->setFileProblem("The specified workspace type (" +
                                               QString::fromStdString(workspace->id()) +
                                               ") is "
                                               "not one of the allowed types: " +
                                               workspaceTypes.join(", "));
        }
      }
    }
  } else {
    isValid = m_uiForm.wsWorkspaceInput->isValid();
  }

  return isValid;
}

/**
 * Return the error.
 * @returns A string explaining the error.
 */
QString DataSelector::getProblem() const {
  QString problem = "";
  if (isFileSelectorVisible()) {
    problem = m_uiForm.rfFileInput->getFileProblem();
    if (problem.compare("") == 0) {
      problem = "Input field is empty";
    }
  } else {
    problem = "A valid workspace has not been selected";
  }

  return problem;
}

/**
 * Attempt to load a file if the widget is set to attempt auto-loading
 *
 * Function creates an instance of the load algorithm and attaches it to a
 * algorithm runner to attempt loading.
 *
 * @param filepath :: The file path to load
 */
void DataSelector::autoLoadFile(const QString &filepath) {
  const auto baseName = getWsNameFromFiles().toStdString();
  executeLoadAlgorithm(filepath.toStdString(), baseName);
}

/**
 * Executes the load algorithm in a background thread using the Algorithm runner
 *
 * @param filename :: The filename of the file to be loaded.
 * @param outputWorkspace :: The name to give the output workspace.
 */
void DataSelector::executeLoadAlgorithm(std::string const &filename, std::string const &outputWorkspace) {
  const auto loadAlg = AlgorithmManager::Instance().createUnmanaged(loadAlgName(filename));
  loadAlg->initialize();
  loadAlg->setProperty("Filename", filename);
  loadAlg->setProperty("OutputWorkspace", outputWorkspace);
  loadAlg->updatePropertyValues(m_loadProperties);

  m_algRunner.startAlgorithm(loadAlg);
}

/**
 * Set an extra property on the load algorithm before execution
 *
 * @param propertyName :: The name of the Load algorithm property to be set
 * @param value :: The value of the Load algorithm property to be set
 */
void DataSelector::setLoadProperty(std::string const &propertyName, bool const value) {
  Mantid::API::AlgorithmProperties::update(propertyName, value, m_loadProperties);
}

/**
 * Handles when the load algorithm completes.
 *
 * @param error :: Whether loading completed without error
 */
void DataSelector::handleAutoLoadComplete(bool error) {
  m_uiForm.rfFileInput->setFileProblem(error ? "Could not load file. See log for details." : "");

  if (error) {
    return;
  }
  if (m_alwaysLoadAsGroup) {
    makeGroup(getWsNameFromFiles().toStdString());
  }
  emit dataReady(getWsNameFromFiles());
}

/**
 * Handles when the user select a workspace in the workspace selector
 */
void DataSelector::handleWorkspaceInput() {
  if (m_uiForm.stackedDataSelect->currentIndex() > 0) {
    // Get text of name of workspace to use
    QString filename = m_uiForm.wsWorkspaceInput->currentText();
    if (filename.isEmpty())
      return;

    // emit that we got a valid workspace/file to work with
    emit dataReady(filename);
  }
}

/**
 * Handles when the view changes between workspace and file selection
 *
 * @param index :: The index the stacked widget has been switched too.
 */
void DataSelector::handleViewChanged(int index) {
  // Index indicates which view is visible.
  m_uiForm.stackedDataSelect->setCurrentIndex(index);

  // 0 is always file view
  switch (index) {
  case 0:
    emit fileViewVisible();
    break;
  case 1:
    emit workspaceViewVisible();
    handleWorkspaceInput();
    break;
  }
}

/**
 * Gets the full file path currently in the file browser
 *
 * @return The full file path
 */
QString DataSelector::getFullFilePath() const { return m_uiForm.rfFileInput->getUserInput().toString(); }

/**
 * Gets the workspace name that is created after loading the files
 *
 * @return The workspace name that is created after loading the files
 */
QString DataSelector::getWsNameFromFiles() const {
  QString filepath = DataSelector::getFullFilePath();
  QFileInfo qfio(filepath);
  QString baseName = qfio.completeBaseName();

  // make up a name for the group workspace, if multiple files are specified
  if (m_uiForm.rfFileInput->allowMultipleFiles() && filepath.count(",") > 0) {
    baseName += "_group";
  }

  return baseName;
}

/**
 * Gets the name of item selected in the DataSelector.
 *
 * This will return the currently selected item in the workspace selector,
 * if the workspace view is active.
 * If the file view is active, it will return the basename of the file.
 * If multiple files are allowed, and auto-loading is off, it will return the
 * full user input.
 * If there is no valid input the method returns an empty string.
 *
 * @return The name of the current data item
 */
QString DataSelector::getCurrentDataName() const {
  QString filename("");

  int index = m_uiForm.stackedDataSelect->currentIndex();

  switch (index) {
  case 0:
    // the file selector is visible
    if (m_uiForm.rfFileInput->isValid()) {
      if (m_uiForm.rfFileInput->allowMultipleFiles() && !m_autoLoad) {
        // if multiple files are allowed, auto-loading is not on, return the
        // full user input
        filename = getFullFilePath();
      } else {
        filename = getWsNameFromFiles();
      }
    }
    break;
  case 1:
    // the workspace selector is visible
    filename = m_uiForm.wsWorkspaceInput->currentText();
    break;
  }

  return filename;
}

/**
 * Gets whether the widget will attempt to auto load files
 *
 * @return Whether the widget will auto load
 */
bool DataSelector::willAutoLoad() const { return m_autoLoad; }

/**
 * Sets whether the widget will attempt to auto load files.
 *
 * @param load :: Whether the widget will auto load
 */
void DataSelector::setAutoLoad(bool load) { m_autoLoad = load; }

/**
 * Gets the text displayed on the load button
 *
 * @return The text on the load button
 */
QString DataSelector::getLoadBtnText() const { return m_uiForm.pbLoadFile->text(); }

/**
 * Sets the text shown on the load button.
 *
 * @param text :: The text to display on the button
 */
void DataSelector::setLoadBtnText(const QString &text) { m_uiForm.pbLoadFile->setText(text); }

/**
 * Sets the DataSelector to always make sure the loaded data is a WorkspaceGroup. If only one entry is loaded from a
 * NeXus file, then this entry is added to a WorkspaceGroup.
 *
 * @param loadAsGroup :: Whether to load the data as a workspace group.
 */
void DataSelector::setAlwaysLoadAsGroup(bool const loadAsGroup) { m_alwaysLoadAsGroup = loadAsGroup; }

/**
 * Read settings from the given group
 * @param group :: The name of the group key to retrieve data from
 */
void DataSelector::readSettings(const QString &group) { m_uiForm.rfFileInput->readSettings(group); }

/**
 * Save settings to the given group
 * @param group :: The name of the group key to save to
 */
void DataSelector::saveSettings(const QString &group) { m_uiForm.rfFileInput->saveSettings(group); }

/**
 * Check if the load button will be shown on the interface
 *
 * @return If the load button will be shown or not
 */
bool DataSelector::willShowLoad() { return m_showLoad; }

/**
 * Set if the load button will be shown or not
 * This will change if the button widget is visible and enabled.
 *
 * @param load :: Whether the load button will be shown
 */
void DataSelector::setShowLoad(bool load) {
  m_uiForm.pbLoadFile->setEnabled(load);
  m_uiForm.pbLoadFile->setVisible(load);
  m_showLoad = load;
}

/**
 * Called when an item is dropped
 * @param de :: the drop event data package
 */
void DataSelector::dropEvent(QDropEvent *de) {
  const QMimeData *mimeData = de->mimeData();
  auto before_action = de->dropAction();

  auto const dragData = mimeData->text().toStdString();

  if (de->mimeData() && ads.doesExist(dragData)) {
    m_uiForm.wsWorkspaceInput->dropEvent(de);
    if (de->dropAction() == before_action) {
      setWorkspaceSelectorIndex(mimeData->text());
      m_uiForm.cbInputType->setCurrentIndex(1);
      return;
    }
    de->setDropAction(before_action);
  }

  m_uiForm.rfFileInput->dropEvent(de);
  if (de->dropAction() == before_action) {
    m_uiForm.cbInputType->setCurrentIndex(0);
  }

  auto const filepath = m_uiForm.rfFileInput->getText().toStdString();
  if (de->mimeData() && !ads.doesExist(dragData) && !filepath.empty()) {
    auto const file = extractLastOf(filepath, "/");
    if (fileFound(file)) {
      auto const workspaceName = cutLastOf(file, ".");
      executeLoadAlgorithm(filepath, workspaceName);

      setWorkspaceSelectorIndex(QString::fromStdString(workspaceName));
      m_uiForm.cbInputType->setCurrentIndex(1);
    }
  }
}

/**
 * This sets the workspace selector's index to the workspace which was dragged
 * and dropped onto the data selector
 * @param workspaceName :: the name of the workspace dragged onto the selector
 */
void DataSelector::setWorkspaceSelectorIndex(QString const &workspaceName) {
  auto const index = m_uiForm.wsWorkspaceInput->findText(workspaceName);
  m_uiForm.wsWorkspaceInput->setCurrentIndex(index != -1 ? index : 0);
}

/**
 * Called when an item is dragged onto a control
 * @param de :: the drag event data package
 */
void DataSelector::dragEnterEvent(QDragEnterEvent *de) {
  const QMimeData *mimeData = de->mimeData();
  if (mimeData->hasText() || mimeData->hasUrls())
    de->acceptProposedAction();
}

} // namespace MantidQt::MantidWidgets
