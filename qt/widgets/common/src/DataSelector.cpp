#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Exception.h"

#include <QFileInfo>

#include <QDebug>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

namespace MantidQt {
namespace MantidWidgets {

DataSelector::DataSelector(QWidget *parent)
    : API::MantidWidget(parent), m_algRunner(), m_autoLoad(true),
      m_showLoad(true) {
  m_uiForm.setupUi(this);
  connect(m_uiForm.cbInputType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleViewChanged(int)));
  connect(m_uiForm.pbLoadFile, SIGNAL(clicked()), this, SIGNAL(loadClicked()));

  // data selected changes
  connect(m_uiForm.rfFileInput, SIGNAL(filesFoundChanged()), this,
          SLOT(handleFileInput()));
  connect(m_uiForm.wsWorkspaceInput, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleWorkspaceInput()));
  connect(m_uiForm.pbLoadFile, SIGNAL(clicked()), this,
          SLOT(handleFileInput()));

  connect(&m_algRunner, SIGNAL(algorithmComplete(bool)), this,
          SLOT(handleAutoLoadComplete(bool)));
  this->setAcceptDrops(true);
  m_uiForm.rfFileInput->setAcceptDrops(false);
}

DataSelector::~DataSelector() {}

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
    autoLoadFile(filename);
  } else {
    // files were found
    emit filesFound();
  }
}

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
bool DataSelector::isWorkspaceSelectorVisible() const {
  return !isFileSelectorVisible();
}

/**
 * Check if the data selector is in a valid state
 *
 * Checks using the relvant widgets isValid method depending
 * on what view is currently being shown
 *
 * @return :: If the data selector is valid
 */
bool DataSelector::isValid() {
  using namespace Mantid::API;

  bool isValid = false;

  if (isFileSelectorVisible()) {
    isValid = m_uiForm.rfFileInput->isValid();

    // check to make sure the user hasn't deleted the auto-loaded file
    // since choosing it.
    if (isValid && m_autoLoad) {
      const QString wsName = getCurrentDataName();

      if (!AnalysisDataService::Instance().doesExist(wsName.toStdString())) {
        // attempt to reload if we can
        // don't use algorithm runner because we need to know instantly.
        const QString filepath =
            m_uiForm.rfFileInput->getUserInput().toString();
        const Algorithm_sptr loadAlg =
            AlgorithmManager::Instance().createUnmanaged("Load");
        loadAlg->initialize();
        loadAlg->setProperty("Filename", filepath.toStdString());
        loadAlg->setProperty("OutputWorkspace", wsName.toStdString());
        loadAlg->execute();

        isValid =
            AnalysisDataService::Instance().doesExist(wsName.toStdString());

        if (!isValid) {
          m_uiForm.rfFileInput->setFileProblem("The specified workspace is "
                                               "missing from the analysis data "
                                               "service");
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
  using namespace Mantid::API;

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
  using namespace Mantid::API;
  QString baseName = getWsNameFromFiles();

  // create instance of load algorithm
  const Algorithm_sptr loadAlg =
      AlgorithmManager::Instance().createUnmanaged("Load");
  loadAlg->initialize();
  loadAlg->setProperty("Filename", filepath.toStdString());
  loadAlg->setProperty("OutputWorkspace", baseName.toStdString());

  m_algRunner.startAlgorithm(loadAlg);
}

/**
 * Handles when the load algorithm completes.
 *
 * @param error :: Whether loading completed without error
 */
void DataSelector::handleAutoLoadComplete(bool error) {
  if (!error) {

    // emit that we got a valid workspace/file to work with
    emit dataReady(getWsNameFromFiles());
  } else {
    m_uiForm.rfFileInput->setFileProblem(
        "Could not load file. See log for details.");
  }
}

/**
 * Handles when the user select a workspace in the workspace selector
 */
void DataSelector::handleWorkspaceInput() {
  if (m_uiForm.stackedDataSelect->currentIndex() > 0) {
    // Get text of name of workspace to use
    QString filename = m_uiForm.wsWorkspaceInput->currentText();
    if (filename.isEmpty()) {
      return;
    }

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
QString DataSelector::getFullFilePath() const {
  return m_uiForm.rfFileInput->getUserInput().toString();
}

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
QString DataSelector::getLoadBtnText() const {
  return m_uiForm.pbLoadFile->text();
}

/**
 * Sets the text shown on the load button.
 *
 * @param text :: The text to display on the button
 */
void DataSelector::setLoadBtnText(const QString &text) {
  m_uiForm.pbLoadFile->setText(text);
}

/**
 * Read settings from the given group
 * @param group :: The name of the group key to retrieve data from
 */
void DataSelector::readSettings(const QString &group) {
  m_uiForm.rfFileInput->readSettings(group);
}

/**
 * Save settings to the given group
 * @param group :: The name of the group key to save to
 */
void DataSelector::saveSettings(const QString &group) {
  m_uiForm.rfFileInput->saveSettings(group);
}

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

  if (de->mimeData() && mimeData->text().contains(" = mtd[\"")) {
    m_uiForm.wsWorkspaceInput->dropEvent(de);
    if (de->dropAction() == before_action) {
      m_uiForm.cbInputType->setCurrentIndex(1);
      return;
    }
    de->setDropAction(before_action);
  }

  m_uiForm.rfFileInput->dropEvent(de);
  if (de->dropAction() == before_action) {
    m_uiForm.cbInputType->setCurrentIndex(0);
  }
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

} /* namespace MantidWidgets */
} /* namespace MantidQt */
