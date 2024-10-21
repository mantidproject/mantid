// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Common/SaveWorkspaces.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"

#include <QCloseEvent>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QShowEvent>
#include <QVBoxLayout>

namespace {
void setDetectorNamesOnCanSasFormat(QString &saveCommands, const QList<QListWidgetItem *> &wspaces, int j) {
  saveCommands += ", DetectorNames=";
  Mantid::API::Workspace_sptr workspace_ptr =
      Mantid::API::AnalysisDataService::Instance().retrieve(wspaces[j]->text().toStdString());
  Mantid::API::MatrixWorkspace_sptr matrix_workspace =
      std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace_ptr);
  if (matrix_workspace) {
    if (matrix_workspace->getInstrument()->getName() == "SANS2D")
      saveCommands += "'front-detector, rear-detector'";
    if (matrix_workspace->getInstrument()->getName() == "LOQ")
      saveCommands += "'HAB, main-detector-bank'";
  }
}
} // namespace

using namespace MantidQt::MantidWidgets;
using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------
// Public member functions
//----------------------
/**
 *  @param parent :: used by QT
 *  @param suggFname :: sets the initial entry in the filename box
 *  @param defSavs :: sets which boxes are ticked
 *  @param saveAsZeroErrorFree :: if the workspace should be saved with the zero
 * errors replaced by a default value or not
 */
SaveWorkspaces::SaveWorkspaces(QWidget *parent, const QString &suggFname,
                               QHash<const QCheckBox *const, QString> &defSavs, bool saveAsZeroErrorFree)
    : API::MantidDialog(parent), m_saveAsZeroErrorFree(saveAsZeroErrorFree), m_geometryID(""), m_sampleHeight(""),
      m_sampleWidth(""), m_sampleThickness("") {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle("Save Workspaces");

  // the form is split into 3 lines of controls in horizontal layouts
  auto *lineOne = new QHBoxLayout;
  setupLine1(lineOne);
  auto *lineTwo = new QHBoxLayout;
  setupLine2(lineTwo, defSavs);

  auto *dialogLayout = new QVBoxLayout;
  dialogLayout->addLayout(lineOne);
  dialogLayout->addLayout(lineTwo);

  setLayout(dialogLayout);

  readSettings();
  setFileName(suggFname);
}
/// Set up the dialog layout
void SaveWorkspaces::initLayout() {}
/** Puts the controls that go on the first line, the output
 *  filename commands, on to the layout that's passed to it
 *  @param lineOne :: the layout on to which the controls will be placed
 */
void SaveWorkspaces::setupLine1(QHBoxLayout *const lineOne) {
  auto *fNameLabel = new QLabel("Filename:");
  m_fNameEdit = new QLineEdit();
  auto *fNameButton = new QPushButton("Browse");
  connect(fNameButton, SIGNAL(clicked()), this, SLOT(saveFileBrowse()));

  lineOne->addWidget(fNameLabel);
  lineOne->addWidget(m_fNameEdit);
  lineOne->addWidget(fNameButton);

  fNameLabel->setToolTip("Filename to save under");
  m_fNameEdit->setToolTip("Filename to save under");
  fNameButton->setToolTip("Filename to save under");
}
/** Puts the controls that go on the second line, the workspace
 *  list and save commands, on to the layout that's passed to it
 *  @param lineTwo :: the layout on to which the controls will be placed
 *  @param defSavs the formats to save into, sets the check boxes to be checked
 */
void SaveWorkspaces::setupLine2(QHBoxLayout *const lineTwo, const QHash<const QCheckBox *const, QString> &defSavs) {
  m_workspaces = new QListWidget();
  auto ws = AnalysisDataService::Instance().getObjectNames();
  auto it = ws.begin(), wsEnd = ws.end();
  for (; it != wsEnd; ++it) {
    Workspace *wksp = FrameworkManager::Instance().getWorkspace(*it);
    if (dynamic_cast<MatrixWorkspace *>(wksp)) { // only include matrix workspaces, not groups or tables
      m_workspaces->addItem(QString::fromStdString(*it));
    }
  }
  // allow users to select more than one workspace in the list
  m_workspaces->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(m_workspaces, SIGNAL(currentRowChanged(int)), this, SLOT(setFileName(int)));

  auto *save = new QPushButton("Save");
  connect(save, SIGNAL(clicked()), this, SLOT(saveSel()));
  auto *cancel = new QPushButton("Cancel");
  connect(cancel, SIGNAL(clicked()), this, SLOT(close()));

  auto *saveRKH = new QCheckBox("RKH (1D/2D)");
  auto *saveNXcanSAS = new QCheckBox("NXcanSAS (1D/2D)");
  auto *saveCan = new QCheckBox("CanSAS (1D)");

  // link the save option tick boxes to their save algorithm
  m_savFormats.insert(saveRKH, "SaveRKH");
  m_savFormats.insert(saveNXcanSAS, "SaveNXcanSAS");
  m_savFormats.insert(saveCan, "SaveCanSAS1D");
  setupFormatTicks(defSavs);

  m_append = new QCheckBox("Append");

  // place controls into the layout, which places them on the form and takes
  // care of deleting them
  auto *ly_saveConts = new QVBoxLayout;
  ly_saveConts->addWidget(save);
  ly_saveConts->addWidget(cancel);
  ly_saveConts->addWidget(m_append);
  ly_saveConts->addStretch();

  auto *ly_saveFormats = new QVBoxLayout;
  ly_saveFormats->addWidget(saveRKH);
  ly_saveFormats->addWidget(saveNXcanSAS);
  ly_saveFormats->addWidget(saveCan);
  auto *gb_saveForms = new QGroupBox(tr("Save Formats"));
  gb_saveForms->setLayout(ly_saveFormats);
  ly_saveConts->addWidget(gb_saveForms);

  lineTwo->addWidget(m_workspaces);
  lineTwo->addLayout(ly_saveConts);

  m_workspaces->setToolTip("Select one or more workspaces");
  const QString formatsTip = "Some formats support appending multiple workspaces in one file";
  gb_saveForms->setToolTip(formatsTip);
  save->setToolTip(formatsTip);
  cancel->setToolTip(formatsTip);
  saveNXcanSAS->setToolTip(formatsTip);
  saveCan->setToolTip(formatsTip);
  saveRKH->setToolTip(formatsTip);
  m_append->setToolTip(formatsTip);
}
/** Sets up some controls from what is in the QSettings
 */
void SaveWorkspaces::readSettings() {
  QSettings prevValues;
  prevValues.beginGroup("CustomInterfaces/SANSRunWindow/SaveWorkspaces");
  m_lastName = prevValues.value("out_name", "").toString();
  m_append->setChecked(prevValues.value("append", false).toBool());
}
/** Set the name of the output file
 *  @param newName filename to use
 */
void SaveWorkspaces::setFileName(const QString &newName) {
  if ((!m_append->isChecked()) && (!newName.isEmpty())) {
    m_fNameEdit->setText(newName);
    m_lastName = newName;
  } else {
    m_fNameEdit->setText(m_lastName);
  }
}
/** For each save format tick box take the user setting from the
 * main form
 * @param defSavs the formats to save into
 */
void SaveWorkspaces::setupFormatTicks(const QHash<const QCheckBox *const, QString> &defSavs) {
  for (SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end(); ++i) {
    // find the setting that has been passed for this save format
    QHash<const QCheckBox *const, QString>::const_iterator j = defSavs.begin();
    for (; j != defSavs.end(); ++j) {
      // the values are the algorithm names
      if (j.value() == i.value()) { // copy over the checked status of the check box
        i.key()->setChecked(j.key()->isChecked());
      }
    }
  }
}
/** Saves the state of some controls to the QSettings
 */
void SaveWorkspaces::saveSettings() const {
  QSettings prevValues;
  prevValues.beginGroup("CustomInterfaces/SANSRunWindow/SaveWorkspaces");
  prevValues.setValue("out_name", m_lastName);
  prevValues.setValue("append", m_append->isChecked());
}
/**
 * Called in response to a close event
 * @param event The event object
 */
void SaveWorkspaces::closeEvent(QCloseEvent *event) {
  saveSettings();
  emit closing();
  event->accept();
}
QString SaveWorkspaces::saveList(const QList<QListWidgetItem *> &wspaces, const QString &algorithm, QString fileBase,
                                 bool toAppend, QHash<QString, QString> workspaceMap) {
  if (wspaces.count() < 1) {
    throw std::logic_error("");
  }

  if (toAppend && fileBase.isEmpty()) { // no file name was given, use the name
                                        // of the first workspace
    fileBase = wspaces[0]->text();
  }
  QString exten = getSaveAlgExt(algorithm);

  QString saveCommands;
  for (int j = 0; j < wspaces.count(); ++j) {
    if (workspaceMap.count(wspaces[j]->text())) {
      saveCommands += algorithm + "('" + workspaceMap[wspaces[j]->text()] + "','";
    } else {
      saveCommands += algorithm + "('" + wspaces[j]->text() + "','";
    }

    QString outFile = fileBase;
    if (outFile.isEmpty()) { // if no filename was given use the workspace names
      outFile = wspaces[j]->text();
    } else {                                      // we have a file name
      if ((wspaces.count() > 1) && (!toAppend)) { // but multiple output files, number the files
        if (outFile.endsWith(exten)) {            // put the number before the extension
          outFile = outFile.split(exten)[0];
        }
        outFile += "-" + QString::number(j + 1);
      }
    }

    if (!outFile.endsWith(exten)) { // code above sometimes removes the
                                    // extension and the possiblity that one
                                    // just wasn't added needs dealing with too
      outFile += exten;
    }
    saveCommands += outFile + "'";
    if (algorithm != "SaveNXcanSAS") {
      saveCommands += ", Append=";
      saveCommands += toAppend ? "True" : "False";
    }
    if (algorithm == "SaveCanSAS1D") {
      setDetectorNamesOnCanSasFormat(saveCommands, wspaces, j);

      // Add the geometry information
      emit updateGeometryInformation();
      // Remove the first three characters, since they are unwanted
      saveCommands += ", Geometry='" + m_geometryID + "', SampleHeight=" + m_sampleHeight +
                      ", SampleWidth=" + m_sampleWidth + ", SampleThickness=" + m_sampleThickness;
    }
    if (algorithm == "SaveNXcanSAS") {
      setDetectorNamesOnCanSasFormat(saveCommands, wspaces, j);
    }
    saveCommands += ")\n";
  }
  return saveCommands;
}
/** Gets the first extension that the algorithm passed algorithm has in it's
 *  FileProperty (the FileProperty must have the name "Filename"
 *  @param algName :: name of the Mantid save algorithm
 *  @return the first extension, if the algorithm's Filename property has an
 * extension list or ""
 */
QString SaveWorkspaces::getSaveAlgExt(const QString &algName) {
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(algName.toStdString());
  Property *prop = alg->getProperty("Filename");
  FileProperty *fProp = dynamic_cast<FileProperty *>(prop);
  if (fProp) {
    return QString::fromStdString(fProp->getDefaultExt());
  } else { // the algorithm doesn't have a "Filename" file property which may
           // indicate an error later on or maybe OK
    return "";
  }
}
/** Excutes the selected save algorithms on the workspaces that
 *  have been selected to be saved
 */
void SaveWorkspaces::saveSel() {
  // Check if the save selection is valid
  if (!isValid()) {
    return;
  }

  // For each selected workspace, provide an zero-error free clone
  QHash<QString, QString> workspaceMap = provideZeroFreeWorkspaces(m_workspaces);

  QString saveCommands;
  for (SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end(); ++i) { // the key to a pointer to the check
                                                                                   // box that the user may have clicked
    if (i.key()->isChecked()) {                                                    // we need to save in this format

      bool toAppend = m_append->isChecked();

      try {
        saveCommands += saveList(m_workspaces->selectedItems(), i.value(), m_fNameEdit->text(), toAppend, workspaceMap);
      } catch (std::logic_error &) {
        QMessageBox::information(this, "No workspace to save", "You must select at least one workspace to save");
        return;
      }
    } // end if save in this format
  } // end loop over formats

  saveCommands += "print('success')";
  QString status(runPythonCode(saveCommands).trimmed());

  if (m_saveAsZeroErrorFree) {
    removeZeroFreeWorkspaces(workspaceMap);
  }

  if (status != "success") {
    QMessageBox::critical(this, "Error saving workspace",
                          "One of the workspaces could not be saved in one of "
                          "the selected formats");
  }
}

/**
 * Checks if the save option selection is compatible with the dimensionality
 * selection
 * @return true if the save option selection is compatible with the
 * dimensionality selection else false
 */
bool SaveWorkspaces::isValid() {
  // Get the dimensionality of the workspaces
  auto is2D = false;
  auto workspacesList = m_workspaces->selectedItems();
  for (auto &it : workspacesList) {
    auto wsName = it->text();
    auto workspace = AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(wsName.toStdString());
    if (workspace->getNumberHistograms() != 1) {
      is2D = true;
    }
  }

  // Check if CanSAS was selected
  auto isCanSAS = false;
  for (SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end(); ++i) { // the key to a pointer to the check
                                                                                   // box that the user may have clicked
    if (i.key()->isChecked()) {                                                    // we need to save in this format
      if (i.value() == "SaveCanSAS1D") {
        isCanSAS = true;
      }
    }
  }

  // Check for errors
  QString message;
  auto isValidOption = true;

  if (is2D && isCanSAS) {
    isValidOption = false;
    message += "Save option issue: Cannot save in CanSAS format for 2D data.\n";
  }

  // Print the error message if there are any
  if (!message.isEmpty()) {
    QString warning = "Please correct these save settings before proceeding:\n";
    warning += message;
    QMessageBox::warning(this, "Inconsistent input", warning);
  }

  return isValidOption;
}

/** Sets the filename to the name of the selected workspace
 *  @param row number of the row that is selected
 */
void SaveWorkspaces::setFileName(int row) { setFileName(m_workspaces->item(row)->text()); }
/** Raises a browse dialog and inserts the selected file into the
 *  save text edit box, outfile_edit
 */
void SaveWorkspaces::saveFileBrowse() {
  QString title = "Save output workspace as";

  QSettings prevValues;
  prevValues.beginGroup("CustomInterfaces/SANSRunWindow/SaveWorkspaces");
  // use their previous directory first and go to their default if that fails
  QString prevPath =
      prevValues.value("dir", QString::fromStdString(ConfigService::Instance().getString("defaultsave.directory")))
          .toString();

  QString filter = ";;AllFiles (*)";
  QFileDialog::Option userCon =
      m_append->isChecked() ? QFileDialog::DontConfirmOverwrite : static_cast<QFileDialog::Option>(0);
  QString oFile = QFileDialog::getSaveFileName(this, title, prevPath, filter, nullptr, userCon);

  if (!oFile.isEmpty()) {
    m_fNameEdit->setText(oFile);

    QString directory = QFileInfo(oFile).path();
    prevValues.setValue("dir", directory);
  }
}

/**
 * Goes through all selected workspaces and maps them to a zero-error free
 * clone,
 * if the user has selected to do this otherwise the value of the hash is set to
 * the same as the key
 * @param workspaces :: a QListWIdget which contains the selected workspaces
 * @returns a hash which maps the original workspace to the zero-error free
 * workspace
 */
QHash<QString, QString> SaveWorkspaces::provideZeroFreeWorkspaces(const QListWidget *workspaces) {
  auto wsList = workspaces->selectedItems();
  QHash<QString, QString> workspaceMap;
  for (auto &it : wsList) {
    auto wsName = it->text();
    auto cloneName = wsName;
    if (m_saveAsZeroErrorFree) {
      cloneName += "_clone_temp";
      emit createZeroErrorFreeWorkspace(wsName, cloneName);
    }

    if (AnalysisDataService::Instance().doesExist(cloneName.toStdString())) {
      workspaceMap.insert(wsName, cloneName);
    }
  }

  return workspaceMap;
}

/**
 * Remove all the zero-error free workspaces
 * @param workspaces :: a map containing the names of all zero-error-free
 * workspaces.
 */
void SaveWorkspaces::removeZeroFreeWorkspaces(const QHash<QString, QString> &workspaces) {
  auto zeroFreeWorkspaceNames = workspaces.values();
  for (auto &zeroFreeWorkspaceName : zeroFreeWorkspaceNames) {
    emit deleteZeroErrorFreeWorkspace(zeroFreeWorkspaceName);
  }
}

/**
 * Reacts to a user change wether the workspace is to be saved as
 * zero-error-free or not
 * @param state :: 0 if we don't save with the zero-error correction, otherwise
 * anything else
 */
void SaveWorkspaces::onSaveAsZeroErrorFreeChanged(int state) {
  if (state == 0) {
    m_saveAsZeroErrorFree = false;
  } else {
    m_saveAsZeroErrorFree = true;
  }
}

/**
 * Recieves an update for the geometry information
 */
void SaveWorkspaces::onUpdateGeomtryInformation(QString &geometryID, QString &sampleHeight, QString &sampleWidth,
                                                QString &sampleThickness) {
  m_geometryID = geometryID;
  m_sampleHeight = sampleHeight;
  m_sampleWidth = sampleWidth;
  m_sampleThickness = sampleThickness;
}
