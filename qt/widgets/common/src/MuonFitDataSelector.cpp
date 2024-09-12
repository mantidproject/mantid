// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MuonFitDataSelector.h"
#include "MantidKernel/Logger.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#include <QFileInfo>

namespace {
Mantid::Kernel::Logger g_log("MuonFitDataSelector");
}

namespace MantidQt {
namespace MantidWidgets {

/**
 * Basic constructor
 * @param parent :: [input] Parent dialog for the widget
 */
MuonFitDataSelector::MuonFitDataSelector(QWidget *parent) : MantidWidget(parent) {
  m_multiFit = false;
  m_ui.setupUi(this);
  this->setDefaultValues();
  this->setUpConnections();
  // Disable "Browse" button - use case is that first run will always be the one
  // selected on front tab. User will type in the runs they want rather than
  // using the Browse button. (If they want to "Browse" they can use front tab).
  m_ui.runs->doButtonOpt(API::FileFinderWidget::ButtonOpts::None);
}

/**
 * Constructor for the widget
 * @param parent :: [input] Parent dialog for the widget
 * @param runNumber :: [input] Run number of initial workspace
 * @param instName :: [input] Name of instrument from initial workspace
 */
MuonFitDataSelector::MuonFitDataSelector(QWidget *parent, int runNumber, const QString &instName) /*
                                  * numPeriods :: [input] Number of periods from initial workspace
                                  * groups :: [input] Group names from initial workspace
                                                                          size_t numPeriods,
                                                                          const QStringList &groups)*/
    : MuonFitDataSelector(parent) {
  m_multiFit = false;
  this->setWorkspaceDetails(QString::number(runNumber), instName, std::optional<QString>{});
  // not used in this case
  // but leave these here as a remainder
  // for future changes that may need to assign them

  // this->setNumPeriods(numPeriods);
  // this->setAvailableGroups(groups);
}

/**
 * Connect signals from UI elements to re-emit a signal that "user has changed
 * something"
 */
void MuonFitDataSelector::setUpConnections() {
  connect(m_ui.runs, SIGNAL(filesFound()), this, SLOT(userChangedRuns()));
  connect(m_ui.rbCoAdd, SIGNAL(toggled(bool)), this, SLOT(fitTypeChanged(bool)));
  connect(m_ui.txtSimFitLabel, SIGNAL(editingFinished()), this, SIGNAL(simulLabelChanged()));
  connect(this, SIGNAL(workspaceChanged()), this, SLOT(checkForMultiGroupPeriodSelection()));
  connect(m_ui.cbDataset, SIGNAL(currentIndexChanged(int)), this, SIGNAL(datasetIndexChanged(int)));
  connect(m_ui.cbDataset, SIGNAL(currentIndexChanged(int)), this, SLOT(updateNormalizationFromDropDown(int)));
  connect(m_ui.btnNextDataset, SIGNAL(clicked()), this, SLOT(setNextDataset()));
  connect(m_ui.btnPrevDataset, SIGNAL(clicked()), this, SLOT(setPreviousDataset()));
}

/**
 * Called when fit type changed. Emit a signal.
 * @param state :: [input] Unused.
 */
void MuonFitDataSelector::fitTypeChanged(bool state) {
  (void)state;
  emit workspaceChanged();
}

/**
 * Slot: called when user edits runs box.
 * Check for single run and enable/disable radio buttons,
 * and emit signal that runs have changed.
 */
void MuonFitDataSelector::userChangedRuns() {
  // check for single run and enable/disable radio buttons
  const auto runs = getRuns();
  if (runs.contains(',') || runs.contains('-')) {
    // record if its multi fit
    m_multiFit = true;
  } else {
    setFitType(FitType::Single);
  }
  emit workspaceChanged();
}

/**
 * Get the user's supplied start time (default 0)
 * @returns :: start time input by user in microseconds
 */
double MuonFitDataSelector::getStartTime() const {
  // Validator ensures cast to double will succeed
  return m_startX; // start.toDouble();
}

/**
 * Set the start time in the UI WITHOUT sending signal
 * @param start :: [input] Start time in microseconds
 */
void MuonFitDataSelector::setStartTimeQuietly(double start) { m_startX = start; }

/**
 * Set the start time in the UI, and send signal
 * @param start :: [input] Start time in microseconds
 */
void MuonFitDataSelector::setStartTime(double start) {
  setStartTimeQuietly(start);
  emit dataPropertiesChanged();
}

/**
 * Get the user's supplied end time (default 10)
 * @returns :: start time input by user in microseconds
 */
double MuonFitDataSelector::getEndTime() const { return m_endX; }

/**
 * Set the end time in the UI WITHOUT sending signal
 * @param end :: [input] End time in microseconds
 */
void MuonFitDataSelector::setEndTimeQuietly(double end) { m_endX = end; }

/**
 * Set the end time in the UI, and send signal
 * @param end :: [input] End time in microseconds
 */
void MuonFitDataSelector::setEndTime(double end) {
  setEndTimeQuietly(end);
  emit dataPropertiesChanged();
}

/**
 * Get the filenames of the supplied run numbers
 * @returns :: list of run filenames
 */
QStringList MuonFitDataSelector::getFilenames() const { return m_ui.runs->getFilenames(); }

/**
 * Set up run finder with initial run number and instrument
 * @param runNumbers :: [input] Run numbers from loaded workspace
 * @param instName :: [input] Instrument name from loaded workspace
 * @param filePath :: [input] Optional path to the data file - this is important
 * in the case of "load current run" when the file may have a special name like
 * MUSRauto_E.tmp
 */
void MuonFitDataSelector::setWorkspaceDetails(const QString &runNumbers, const QString &instName,
                                              const std::optional<QString> &filePath) {
  // Set the file finder to the correct instrument (not Mantid's default)
  m_ui.runs->setInstrumentOverride(instName);

  QString runs(runNumbers);
  runs.remove(QRegExp("^[0]*")); // remove leading zeros, if any
  // Set fit type - co-add (as this comes from Home tab) or single
  if (runs.contains('-') || runs.contains(',')) {
    setFitType(FitType::CoAdd);
  } else {
    setFitType(FitType::Single);
  }

  // Set initial run to be run number of the workspace loaded in Home tab
  // and search for filenames. Use busy cursor until search finished.
  setBusyState();

  if (filePath) { // load current run: use special file path
    m_ui.runs->setUserInput(filePath.get());
    m_ui.runs->setText(runs);
  } else { // default
    m_ui.runs->setFileTextWithSearch(runs);
  }
}

/**
 * Set default values in some input controls
 * Defaults copy those previously used in muon fit property browser
 */
void MuonFitDataSelector::setDefaultValues() {
  this->setStartTime(0.0);
  this->setEndTime(0.0);
  m_ui.txtSimFitLabel->setText("0");
  emit simulLabelChanged(); // make sure default "0" is set
}
/**
 * Returns a list of periods and combinations chosen in UI
 * @returns :: list of periods e.g. "1", "3", "1+2-3+4", or "" if single-period
 */
QStringList MuonFitDataSelector::getPeriodSelections() const { return m_chosenPeriods; }

/**
 * Returns a list of the selected groups (checked boxes)
 * @returns :: list of selected groups
 */
QStringList MuonFitDataSelector::getChosenGroups() const { return m_chosenGroups; }

/**
 *Gets user input in the form of a QVariant
 *
 *This is implemented as the "standard" way of getting input from a
 *MantidWidget. In practice it is probably easier to get the input
 *using other methods.
 *
 *The returned QVariant is a QVariantMap of (parameter, value) pairs.
 *@returns :: QVariant containing a QVariantMap
 */
QVariant MuonFitDataSelector::getUserInput() const {
  QVariant ret(QVariant::Map);
  auto map = ret.toMap();
  map.insert("Start", getStartTime());
  map.insert("End", getEndTime());
  map.insert("Runs", getRuns());
  map.insert("Groups", getChosenGroups());
  map.insert("Periods", getPeriodSelections());
  return map;
}

/**
 * Sets user input in the form of a QVariant
 *
 * This is implemented as the "standard" way of setting input in a
 * MantidWidget. In practice it is probably easier to set the input
 * using other methods.
 *
 * This function doesn't support setting runs, groups or periods.
 *
 * The input QVariant is a QVariantMap of (parameter, value) pairs.
 * @param value :: [input] QVariant containing a QVariantMap
 */
void MuonFitDataSelector::setUserInput(const QVariant &value) {
  if (value.canConvert(QVariant::Map)) {
    const auto map = value.toMap();
    if (map.contains("Start")) {
      setStartTime(map.value("Start").toDouble());
    }
    if (map.contains("End")) {
      setEndTime(map.value("End").toDouble());
    }
  }
}

/**
 * Returns the selected fit type.
 * - If only one run is selected, this is a single fit UNLESS multiple
 * groups/periods are selected, in which case it's simultaneous.
 * - If multiple runs are selected, the user has the option of co-adding them or
 * doing a simultaneous fit, chosen via the radio buttons.
 * @returns :: fit type from enum
 */
IMuonFitDataSelector::FitType MuonFitDataSelector::getFitType() const {
  // If radio buttons disabled, it's a single fit unless multiple groups/periods
  // chosen
  if (!m_multiFit) {
    return m_chosenGroups.size() <= 1 && m_chosenPeriods.size() <= 1 ? FitType::Single : FitType::Simultaneous;
  } else {
    // which button is selected
    if (m_ui.rbCoAdd->isChecked()) {
      return FitType::CoAdd;
    } else {
      return FitType::Simultaneous;
    }
  }
}

/**
 * Sets the fit type.
 * If single, disables radio buttons.
 * Otherwise, enables buttons and selects the correct one.
 * @param type :: [input] Fit type to set (from enum)
 */
void MuonFitDataSelector::setFitType(IMuonFitDataSelector::FitType type) {
  if (type == FitType::Single) {
    m_multiFit = false;
  } else {
    m_multiFit = true;

    m_ui.rbCoAdd->setChecked(type == FitType::CoAdd);
    m_ui.rbSimultaneous->setChecked(type == FitType::Simultaneous);
  }
  checkForMultiGroupPeriodSelection();
}
/**
 * Return the instrument name currently set as the override
 * for the data selector
 * @returns :: instrument name
 */
QString MuonFitDataSelector::getInstrumentName() const { return m_ui.runs->getInstrumentOverride(); }

/**
 * Return the runs entered by the user
 * @returns :: run number string if valid, else empty string
 */
QString MuonFitDataSelector::getRuns() const {
  if (m_ui.runs->isValid()) {
    return m_ui.runs->getText();
  } else {
    return "";
  }
}

/**
 * Slot: called when file finding finished. Resets the cursor for this widget
 * back to the normal, non-busy state.
 */
void MuonFitDataSelector::unsetBusyState() {
  disconnect(m_ui.runs, SIGNAL(fileInspectionFinished()), this, SLOT(unsetBusyState()));
  this->setCursor(Qt::ArrowCursor);
}

/**
 * Sets busy cursor and disables input while file search in progress.
 * Connects up slot to reset busy state when search done.
 */
void MuonFitDataSelector::setBusyState() {
  connect(m_ui.runs, SIGNAL(fileInspectionFinished()), this, SLOT(unsetBusyState()));
  this->setCursor(Qt::WaitCursor);
}

/**
 * Get text entered as the simultaneous fit label
 * @returns :: text entered in the textbox
 */
QString MuonFitDataSelector::getSimultaneousFitLabel() const { return m_ui.txtSimFitLabel->text(); }

/**
 * Set text of simultaneous fit label
 * @param label :: [input] Text to set as label
 */
void MuonFitDataSelector::setSimultaneousFitLabel(const QString &label) {
  // do some checks that it is valid
  auto safeLabel = label;
  if (label.indexOf(".") >= 0) {
    QFileInfo file(label);
    safeLabel = file.baseName();
    // trim instrument name
    auto index = safeLabel.indexOf("0");
    safeLabel = safeLabel.mid(index);
    // trim leading zeros
    safeLabel = safeLabel.remove(QRegExp("^[0]*"));
  }
  m_ui.txtSimFitLabel->setText(safeLabel);
}

/**
 * Enable the "Label" textbox if multiple groups or periods are selected.
 * Called when groups/periods selection changes.
 */
void MuonFitDataSelector::checkForMultiGroupPeriodSelection() {
  m_ui.txtSimFitLabel->setEnabled(m_chosenGroups.size() > 1 || m_chosenPeriods.size() > 1 ||
                                  getFitType() == FitType::Simultaneous);
}

/**
 * Return index of currently selected dataset
 * @returns :: index of dataset selected in combobox
 */
int MuonFitDataSelector::getDatasetIndex() const { return m_ui.cbDataset->currentIndex(); }

/**
 * Return name of currently selected dataset
 * @returns :: name of dataset selected in combobox
 */
QString MuonFitDataSelector::getDatasetName() const { return m_ui.cbDataset->currentText(); }

/**
 * Replaces list of dataset names with the supplied list
 * @param datasetNames :: [input] List of names to use
 */
void MuonFitDataSelector::setDatasetNames(const QStringList &datasetNames) {
  const auto selectedName = m_ui.cbDataset->currentText();
  // Turn off signals while names are updated
  m_ui.cbDataset->blockSignals(true);
  m_ui.cbDataset->clear();
  m_ui.cbDataset->addItems(datasetNames);
  m_ui.cbDataset->blockSignals(false);

  // If previously selected name is in new list, set this index again.
  int i = 0;
  while (i < m_ui.cbDataset->count()) {
    if (m_ui.cbDataset->itemText(i) == selectedName) {
      m_ui.cbDataset->setCurrentIndex(i);
      break;
    }
    i++;
  }
  // Otherwise select the first in the list.
  if (i == m_ui.cbDataset->count()) {
    m_ui.cbDataset->setCurrentIndex(0);
  }
}
void MuonFitDataSelector::updateNormalizationFromDropDown(int j) {
  for (int i = 0; i < m_ui.cbDataset->count(); i++) {
    if (i == j) {
      auto name = m_ui.cbDataset->itemText(i);
      emit nameChanged(name);
      return;
    }
  }
}
/**
 * Called when "previous dataset" is clicked.
 * Changes combobox to previous dataset, which will raise an event.
 * If there is no previous dataset, does nothing.
 */
void MuonFitDataSelector::setPreviousDataset() {
  const int index = m_ui.cbDataset->currentIndex();
  if (index > 0) {
    m_ui.cbDataset->setCurrentIndex(index - 1);
  }
}

/**
 * Called when "next dataset" is clicked.
 * Changes combobox to next dataset, which will raise an event.
 * If there is no next dataset, does nothing.
 */
void MuonFitDataSelector::setNextDataset() {
  const int index = m_ui.cbDataset->currentIndex();
  const int maxIndex = m_ui.cbDataset->count() - 1;
  if (index < maxIndex) {
    m_ui.cbDataset->setCurrentIndex(index + 1);
  }
}

/**
 * Ask the user whether to overwrite the fit results with the current label,
 * if those results already exist. Raises a messagebox with a yes/no choice.
 * @returns :: True if user chose to overwrite, false if not.
 */
bool MuonFitDataSelector::askUserWhetherToOverwrite() {
  const int choice =
      QMessageBox::question(this, tr("MantidPlot - Overwrite Warning"),
                            getSimultaneousFitLabel() + tr(" already exists. Do you want to replace it?"),
                            QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape);

  return choice == QMessageBox::Yes;
}

} // namespace MantidWidgets
} // namespace MantidQt
