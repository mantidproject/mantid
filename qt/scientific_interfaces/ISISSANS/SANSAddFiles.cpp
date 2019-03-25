// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "SANSAddFiles.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/PropertyHelper.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "SANSRunWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include <QVariant>

#include <Poco/Exception.h>
#include <Poco/Path.h>

#include <algorithm>

namespace {
enum BINOPTIONS { CUSTOMBINNING, FROMMONITORS, SAVEASEVENTDATA };
}

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
/// static logger for main window
Logger g_log("SANSAddFiles");

/**
 * Helper function used to filter QListWidgetItems based on whether or not
 * they contain only whitespace.
 *
 * @param item :: the QListWidgetItem to check
 *
 * @returns false if the item is empty or contains only whitespace, else true
 */
bool isNonEmptyItem(const QListWidgetItem *item) {
  return item->data(Qt::WhatsThisRole).toString().trimmed().length() > 0;
}
} // namespace

const QString SANSAddFiles::OUT_MSG("Output Directory: ");

SANSAddFiles::SANSAddFiles(QWidget *parent, Ui::SANSRunWindow *ParWidgets)
    : m_SANSForm(ParWidgets), parForm(parent), m_pythonRunning(false),
      m_newOutDir(*this, &SANSAddFiles::changeOutputDir), m_customBinning(""),
      m_customBinningText("Bin Settings: "),
      m_customBinningToolTip("Sets the bin options for custom binning"),
      m_saveEventDataText("Additional Time Shifts: "),
      m_saveEventDataToolTip(
          "Set optional, comma-separated time shifts in seconds.\n"
          "You can either specify non or N-1 time shifts for N files.\n"
          "Note that the time shifts are relative to the time of the workspace "
          "which was added last.") {
  initLayout();

  // get lists of suported extentions
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Load");
  Property *prop = alg->getProperty("Filename");
  m_exts = prop->allowedValues();
  // a log file must be copied across if it was a raw file, find out from the
  // extention if a raw file was selected
  alg = AlgorithmManager::Instance().create("LoadRaw");
  prop = alg->getProperty("Filename");
  m_rawExts = prop->allowedValues();

  ConfigService::Instance().addObserver(m_newOutDir);
}

SANSAddFiles::~SANSAddFiles() {
  try {
    ConfigService::Instance().removeObserver(m_newOutDir);
  } catch (...) {
    // we've cleaned up the best we can, move on
  }
}

// Connect signals and setup widgets
void SANSAddFiles::initLayout() {
  connect(m_SANSForm->new2Add_edit, SIGNAL(returnPressed()), this,
          SLOT(add2Runs2Add()));

  // the runAsPythonScript() signal needs to get to Qtiplot, here it is
  // connected to the parent, which is connected to Qtiplot
  connect(this, SIGNAL(runAsPythonScript(const QString &, bool)), parForm,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  insertListFront("");

  connect(m_SANSForm->toAdd_List, SIGNAL(itemChanged(QListWidgetItem *)), this,
          SLOT(setCellData(QListWidgetItem *)));

  // Unfortunately, three signals are needed to track everything that could
  // happen to our QListWidget; this covers adding and removing items as
  // well changes to existing items and clearing all items.
  connect(m_SANSForm->toAdd_List->model(),
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
          SLOT(enableSumming()));
  connect(m_SANSForm->toAdd_List->model(),
          SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this,
          SLOT(enableSumming()));
  connect(m_SANSForm->toAdd_List->model(), SIGNAL(modelReset()), this,
          SLOT(enableSumming()));

  enableSumming();

  // buttons on the Add Runs tab
  connect(m_SANSForm->add_Btn, SIGNAL(clicked()), this, SLOT(add2Runs2Add()));
  connect(m_SANSForm->sum_Btn, SIGNAL(clicked()), this,
          SLOT(runPythonAddFiles()));
  connect(m_SANSForm->summedPath_Btn, SIGNAL(clicked()), this,
          SLOT(outPathSel()));
  connect(m_SANSForm->browse_to_add_Btn, SIGNAL(clicked()), this,
          SLOT(new2AddBrowse()));
  connect(m_SANSForm->clear_Btn, SIGNAL(clicked()), this, SLOT(clearClicked()));
  connect(m_SANSForm->remove_Btn, SIGNAL(clicked()), this,
          SLOT(removeSelected()));

  setToolTips();

  setOutDir(ConfigService::Instance().getString("defaultsave.directory"));

  // Track changes in the selection of the histogram option
  connect(m_SANSForm->comboBox_histogram_choice,
          SIGNAL(currentIndexChanged(int)), this,
          SLOT(onCurrentIndexChangedForHistogramChoice(int)));

  // Track changes in the overlay options
  m_SANSForm->overlayCheckBox->setEnabled(false);
  m_customBinning = m_SANSForm->eventToHistBinning->text();
  connect(m_SANSForm->overlayCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(onStateChangedForOverlayCheckBox(int)));
}

/** sets tool tip strings for the components on the form
 */
void SANSAddFiles::setToolTips() {
  m_SANSForm->summedPath_lb->setToolTip("The output files from summing the "
                                        "workspaces\nwill be saved to this "
                                        "directory");
  m_SANSForm->summedPath_Btn->setToolTip(
      "Set the directories used both for loading and\nsaving run data");

  m_SANSForm->add_Btn->setToolTip("Click here to do the sum");
  m_SANSForm->clear_Btn->setToolTip("Clear the run files to sum box");
  m_SANSForm->browse_to_add_Btn->setToolTip("Select a run to add to the sum");
  m_SANSForm->new2Add_edit->setToolTip("Select a run to add to the sum");
  m_SANSForm->add_Btn->setToolTip("Select a run to add to the sum");
}
/** Creates a QListWidgetItem with the given text and inserts it
 *  into the list box
 *  @param[in] text the text to insert
 *  @return a pointer to the inserted widget
 */
QListWidgetItem *SANSAddFiles::insertListFront(const QString &text) {
  QListWidgetItem *newItem = new QListWidgetItem(text);
  newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
  m_SANSForm->toAdd_List->insertItem(0, newItem);
  return newItem;
}
/** Sets directory to which files will be saved and the label
 *  that users see
 *  @param dir :: full path of the output directory
 */
void SANSAddFiles::setOutDir(std::string dir) {
  m_outDir = QString::fromStdString(dir);
  m_SANSForm->summedPath_lb->setText(OUT_MSG + m_outDir);
}
/** Update the output directory label if the Mantid system output
 *  directory has changed
 *  @param pDirInfo :: a pointer to an object with the output directory name in
 * it
 */
void SANSAddFiles::changeOutputDir(
    Mantid::Kernel::ConfigValChangeNotification_ptr pDirInfo) {
  if (pDirInfo->key() == "defaultsave.directory") {
    setOutDir(pDirInfo->curValue());
  }
}
/**Moves the entry in the line edit new2Add_edit to the
 *  listbox toAdd_List, expanding any run number lists
 */
void SANSAddFiles::add2Runs2Add() {
  // split comma separated file names or run numbers into a list
  ArrayProperty<std::string> commaSep(
      "unusedName", m_SANSForm->new2Add_edit->text().toStdString());
  const std::vector<std::string> &nam = commaSep;

  for (const auto & i : nam) { // each comma separated item could be a range of run numbers
              // specified with a ':' or '-'
    QStringList ranges;
    std::vector<int> runNumRanges;
    try { // if the entry is in the form 454:456, runNumRanges will be filled
          // with the integers ({454, 455, 456}) otherwise it will throw
      appendValue(i, runNumRanges);
      std::vector<int>::const_iterator num = runNumRanges.begin();
      for (; num != runNumRanges.end(); ++num) {
        ranges.append(QString::number(*num));
      }
    } catch (boost::bad_lexical_cast &) { // this means that we don't have a
                                          // list of integers, treat it as full
                                          // (and valid) filename
      ranges.append(QString::fromStdString(i));
    }

    for (QStringList::const_iterator k = ranges.begin(); k != ranges.end();
         ++k) {
      // Check the file property
      FileProperty search("dummy", k->toStdString(), FileProperty::Load,
                          std::vector<std::string>(), Direction::Input);

      std::string isValid;
      try {
        isValid = search.isValid();
      } catch (Poco::PathSyntaxException) {
        QString message =
            QString("The file entry ") + *k +
            QString(" is not a valid file path on your operating system");
        QMessageBox::critical(this, "Invalid entry for file path", message);
        m_SANSForm->new2Add_edit->clear();
        return;
      }

      // Put the full path in the tooltip so people can see it if they want to
      // do this with the file finding functionality of the FileProperty
      // Don't display the full file path in the box, it's too long
      QListWidgetItem *newL = insertListFront(QFileInfo(*k).fileName());
      newL->setData(Qt::WhatsThisRole, QVariant(*k));

      if (isValid == "") { // this means the file was found
        newL->setToolTip(QString::fromStdString(search.value()));

        // If we don't have an event workspace data set, then we disable the
        // event options
        if (!isEventWorkspace(QString::fromStdString(search.value()))) {
          setBinningOptions(false);
        }
      }
    }
  }
  m_SANSForm->new2Add_edit->clear();
}
/** Executes the add_runs() function inside the SANSadd2 script
 */
void SANSAddFiles::runPythonAddFiles() {
  // Check the validty of the input for the
  if (!checkValidityTimeShiftsForAddedEventFiles()) {
    return;
  }

  if (m_pythonRunning) { // it is only possible to run one python script at a
                         // time
    return;
  }

  if (ConfigService::Instance().getString("defaultsave.directory").empty()) {
    QMessageBox::critical(this, "Setting Required",
                          "Unable to add runs until a default save directory "
                          "has been specified.  Please set this using the "
                          "Manage User Directories dialog.");
    return;
  }

  add2Runs2Add();

  QString code_torun = "import SANSadd2\n";
  code_torun += "print(SANSadd2.add_runs((";
  // there are multiple file list inputs that can be filled in loop through them
  for (int i = 0; i < m_SANSForm->toAdd_List->count(); ++i) {
    QString filename =
        m_SANSForm->toAdd_List->item(i)->data(Qt::WhatsThisRole).toString();
    // allow but do nothing with empty entries
    if (!filename.isEmpty()) {
      // Make sure that the file separators are valid
      filename.replace("\\", "/");
      code_torun += "'" + filename + "',";
    }
  }
  if (code_torun.endsWith(',')) { // we've made a comma separated list, there
                                  // can be no comma at the end
    code_torun.truncate(code_torun.size() - 1);
  }
  // pass the current instrument
  code_torun += "),'" + m_SANSForm->inst_opt->currentText() + "', '";
  QString ext =
      m_SANSForm->file_opt->itemData(m_SANSForm->file_opt->currentIndex())
          .toString();
  code_torun += ext + "'";

  code_torun += ", rawTypes=(";
  std::vector<std::string>::const_iterator end = m_rawExts.end();
  for (std::vector<std::string>::const_iterator j = m_rawExts.begin(); j != end;
       ++j) {
    code_torun += "'" + QString::fromStdString(*j) + "',";
  }
  // remove the comma that would remain at the end of the list
  code_torun.truncate(code_torun.length() - 1);
  code_torun += ")";

  QString lowMem = "True";
  code_torun += ", lowMem=" + lowMem;

  QString overlay = m_SANSForm->overlayCheckBox->isChecked() ? "True" : "False";
  // In case of event data, check if the user either wants
  // 0. Custom historgram binning
  // 1. A binning which is set by the data set
  // 2. To save the actual event data
  switch (m_SANSForm->comboBox_histogram_choice->currentIndex()) {
  case CUSTOMBINNING:
    code_torun += ", binning='" + m_SANSForm->eventToHistBinning->text() + "'";
    break;
  case FROMMONITORS:
    break;
  case SAVEASEVENTDATA:
    code_torun += ", saveAsEvent=True";
    code_torun += ", isOverlay=" + overlay;
    code_torun +=
        ", time_shifts=" +
        createPythonStringList(m_SANSForm->eventToHistBinning->text());
    break;
  default:
    break;
  }

  code_torun += "))\n";

  g_log.debug() << "Executing Python: \n" << code_torun.toStdString() << '\n';

  m_SANSForm->sum_Btn->setEnabled(false);
  m_pythonRunning = true;

  // call the algorithms by executing the above script as Python
  QString status = runPythonCode(code_torun, false);

  // reset the controls and display any errors
  m_SANSForm->sum_Btn->setEnabled(true);
  m_pythonRunning = false;
  if (status.startsWith("The following file has been created:")) {
    QMessageBox::information(this, "Files summed", status);
  } else if (status.startsWith("Error copying log file:")) {
    QMessageBox::warning(this, "Error adding files", status);
  } else {
    if (status.isEmpty()) {
      status = "Could not sum files, there may be more\ninformation in the "
               "Results Log window";
    }
    QMessageBox::critical(this, "Error adding files", status);
  }
}
/** This slot opens a manage user directories dialog to allowing the default
 *  output directory to be changed
 */
void SANSAddFiles::outPathSel() {
  MantidQt::API::ManageUserDirectories::openUserDirsDialog(this);
}
/** This slot opens a file browser allowing a user select files, which is
 * copied into the new2Add_edit ready to be copied to the listbox (toAdd_List)
 */
void SANSAddFiles::new2AddBrowse() {
  QSettings prevVals;
  prevVals.beginGroup("CustomInterfaces/SANSRunWindow/AddRuns");
  // get the previous data input directory or, if there wasn't one, the first
  // directory of on the default load path
  std::string d0 = ConfigService::Instance().getDataSearchDirs()[0];
  QString dir = prevVals.value("InPath", QString::fromStdString(d0)).toString();

  QString fileFilter = "Files (";

  std::vector<std::string>::const_iterator end = m_exts.end();
  for (std::vector<std::string>::const_iterator i = m_exts.begin(); i != end;
       ++i) {
    fileFilter += " *" + QString::fromStdString(*i);
  }

  fileFilter += ")";
  const QStringList files =
      QFileDialog::getOpenFileNames(parForm, "Select files", dir, fileFilter);

  if (!files.isEmpty()) {
    // next time the user clicks browse they will see the directory that they
    // last loaded a file from
    QFileInfo defPath(files[0]);
    prevVals.setValue("InPath", defPath.absoluteDir().absolutePath());
    // join turns the list into a single string with the entries seperated, in
    // this case, by ,
    m_SANSForm->new2Add_edit->setText(files.join(", "));
  }
}
/** Normally in responce to an edit this sets data associated with the cell
 *  to the cells text and removes the tooltip
 */
void SANSAddFiles::setCellData(QListWidgetItem *) {
  QListWidgetItem *editting = m_SANSForm->toAdd_List->currentItem();
  if (editting) {
    editting->setData(Qt::WhatsThisRole, QVariant(editting->text()));
    editting->setToolTip("");
  }
}
/** Called when the clear button is clicked it clears the list of file
 * names to add table
 */
void SANSAddFiles::clearClicked() {
  m_SANSForm->toAdd_List->clear();
  insertListFront("");
  setBinningOptions(true);
}

void SANSAddFiles::removeSelected() {
  QList<QListWidgetItem *> sels = m_SANSForm->toAdd_List->selectedItems();
  while (sels.count() > 0) {
    int selRow = m_SANSForm->toAdd_List->row(sels.front());
    delete m_SANSForm->toAdd_List->takeItem(selRow);
    sels = m_SANSForm->toAdd_List->selectedItems();
  }

  // Check if the remaining files correspond to only event workspaces
  if (!existNonEventFiles()) {
    setBinningOptions(true);
  }
}

/**
 * Enables/disables the "Sum" button based on whether there are files to sum.
 */
void SANSAddFiles::enableSumming() {
  const auto allItems =
      m_SANSForm->toAdd_List->findItems("*", Qt::MatchWildcard);
  const auto nonEmptyItemsCount =
      std::count_if(allItems.begin(), allItems.end(), isNonEmptyItem);

  m_SANSForm->sum_Btn->setEnabled(nonEmptyItemsCount > 1);
}

/**
 * Reacts to changges of the combo box selection for the histogram options for
 * event data
 * @param index the new index of the combo box.
 */
void SANSAddFiles::onCurrentIndexChangedForHistogramChoice(int index) {
  // Set the overlay checkbox enabled or disabled
  // Set the input field enabled or disabled
  switch (index) {
  case CUSTOMBINNING:
    m_SANSForm->overlayCheckBox->setEnabled(false);
    setHistogramUiLogic(m_customBinningText, m_customBinningToolTip,
                        m_customBinning, true);
    break;
  case FROMMONITORS:
    setHistogramUiLogic(m_customBinningText, m_customBinningToolTip,
                        m_customBinning, false);
    setInputEnabled(false);
    break;
  case SAVEASEVENTDATA:
    m_customBinning = this->m_SANSForm->eventToHistBinning->text();
    m_SANSForm->eventToHistBinning->setText("");

    setHistogramUiLogic(m_saveEventDataText, m_saveEventDataToolTip, "", true);
    m_SANSForm->overlayCheckBox->setEnabled(true);

    setInputEnabled(m_SANSForm->overlayCheckBox->isChecked());
    break;
  default:
    setInputEnabled(false);
    break;
  }
}

/**
 * Reacts to changes of the overlay check box when adding event data
 * @param state the state of the check box
 */
void SANSAddFiles::onStateChangedForOverlayCheckBox(int state) {
  setInputEnabled(state != 0);
}

/*
 * Check the validity of the time shift input field for added event files
 */
bool SANSAddFiles::checkValidityTimeShiftsForAddedEventFiles() {
  bool state = true;

  if (m_SANSForm->comboBox_histogram_choice->currentIndex() ==
          SAVEASEVENTDATA &&
      m_SANSForm->overlayCheckBox->isChecked()) {
    QString code_torun = "import ISISCommandInterface as i\n";
    code_torun += "i.check_time_shifts_for_added_event_files(number_of_files=";
    code_torun += QString::number(m_SANSForm->toAdd_List->count() - 1);
    code_torun +=
        ", time_shifts='" + m_SANSForm->eventToHistBinning->text() + "')\n";

    QString status = runPythonCode(code_torun, false);
    if (!status.isEmpty()) {
      g_log.warning() << status.toStdString();
    }

    if (status.contains("Error")) {
      state = false;
    }
  }

  return state;
}

/**
 * Set the UI logic for the histogram binning and saving as event data bit.
 * @param label :: the label of the line edit field.
 * @param toolTip :: the tooltip text.
 * @param lineEditText :: text for the line edit field
 * @param enabled :: if the input should be enabled.
 */
void SANSAddFiles::setHistogramUiLogic(QString label, QString toolTip,
                                       QString lineEditText, bool enabled) {
  // Line edit field
  m_SANSForm->eventToHistBinning->setText(lineEditText);
  m_SANSForm->eventToHistBinning->setToolTip(toolTip);

  // Label for line edit field
  m_SANSForm->binning_label->setText(label);
  m_SANSForm->binning_label->setToolTip(toolTip);

  setInputEnabled(enabled);
}

/**
 * Enables or disables the line editr field for histograms and time shifts, as
 * well
 * as the corresponding labels
 * @param enabled :: is enabled or not
 */
void SANSAddFiles::setInputEnabled(bool enabled) {
  m_SANSForm->eventToHistBinning->setEnabled(enabled);
  m_SANSForm->binning_label->setEnabled(enabled);
}

/**
 * Produces a Python string list of the format "['entry1', 'entry2', ...]"
 * @param inputString :: This string has a format of "entry1, entry2, ..."
 * @returns a Python list of strings
 */
QString SANSAddFiles::createPythonStringList(QString inputString) {
  QString formattedString = "[";
  QString finalizer = "]";
  QString quotationMark = "'";
  QString delimiter = ",";

  if (inputString.isEmpty()) {
    return formattedString + finalizer;
  }

  inputString.replace(" ", "");
  auto inputStringList = inputString.split(delimiter);

  for (auto & it : inputStringList) {

    formattedString += quotationMark + it + quotationMark + delimiter;
  }

  formattedString.remove(formattedString.length() - delimiter.length(),
                         delimiter.length());
  formattedString += finalizer;
  return formattedString;
}

/**
 * Check if a file corresponds to a histogram workspace
 * @param fileName: the file name
 * @returns true if it is a histogram workspace
 */
bool SANSAddFiles::isEventWorkspace(QString fileName) {
  auto isEvent = false;
  fileName.replace("\\", "/");
  QString code_torun = "import ISISCommandInterface as i\n";
  code_torun += "i.check_if_event_workspace(file_name='";
  code_torun += fileName;
  code_torun += +"')\n";

  auto status = runPythonCode(code_torun, false);
  if (status.contains(m_constants.getPythonTrueKeyword())) {
    isEvent = true;
  }
  return isEvent;
}

/**
 * Enable or disable the binning options
 * @param enable: If the options should be enabled or disabled
 */
void SANSAddFiles::setBinningOptions(bool enable) {
  m_SANSForm->eventToHistBinning->setEnabled(enable);
  m_SANSForm->comboBox_histogram_choice->setEnabled(enable);
  m_SANSForm->overlayCheckBox->setEnabled(enable);
  m_SANSForm->histogram_binning_label->setEnabled(enable);
  m_SANSForm->binning_label->setEnabled(enable);
}

/**
 * Check if non-event type files exist
 * returns true if all are event files or if there are no files else false
 */
bool SANSAddFiles::existNonEventFiles() {
  auto elements = m_SANSForm->toAdd_List->count();
  for (int i = 0; i < elements; ++i) {
    auto fileName =
        m_SANSForm->toAdd_List->item(i)->data(Qt::WhatsThisRole).toString();
    if (!fileName.isEmpty()) {
      // Make sure that the file separators are valid
      fileName.replace("\\", "/");
      // Run the check
      if (!isEventWorkspace(fileName)) {
        return true;
      }
    }
  }

  return false;
}

} // namespace CustomInterfaces
} // namespace MantidQt
