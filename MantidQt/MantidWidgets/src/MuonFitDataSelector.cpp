#include "MantidQtMantidWidgets/MuonFitDataSelector.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("MuonFitDataSelector");
}

namespace MantidQt {
namespace MantidWidgets {

/**
 * Basic constructor
 * @param parent :: [input] Parent dialog for the widget
 */
MuonFitDataSelector::MuonFitDataSelector(QWidget *parent)
    : MantidWidget(parent) {
  m_ui.setupUi(this);
  this->setUpValidators();
  this->setDefaultValues();
  this->setUpConnections();
  m_groupBoxes.insert("fwd", m_ui.chkFwd);
  m_groupBoxes.insert("bwd", m_ui.chkBwd);
  m_periodBoxes.insert("1", m_ui.chk1);
  m_periodBoxes.insert("2", m_ui.chk2);

  // Disable "Browse" button - use case is that first run will always be the one
  // selected on front tab. User will type in the runs they want rather than
  // using the Browse button. (If they want to "Browse" they can use front tab).
  m_ui.runs->doButtonOpt(API::MWRunFiles::ButtonOpts::None);
}

/**
 * Constructor for the widget
 * @param parent :: [input] Parent dialog for the widget
 * @param runNumber :: [input] Run number of initial workspace
 * @param instName :: [input] Name of instrument from initial workspace
 * @param numPeriods :: [input] Number of periods from initial workspace
 * @param groups :: [input] Group names from initial workspace
 */
MuonFitDataSelector::MuonFitDataSelector(QWidget *parent, int runNumber,
                                         const QString &instName,
                                         size_t numPeriods,
                                         const QStringList &groups)
    : MuonFitDataSelector(parent) {
  this->setWorkspaceDetails(QString::number(runNumber), instName);
  this->setNumPeriods(numPeriods);
  this->setAvailableGroups(groups);
}

/**
 * Connect signals from UI elements to re-emit a signal that "user has changed
 * something"
 */
void MuonFitDataSelector::setUpConnections() {
  connect(m_ui.runs, SIGNAL(filesFound()), this, SLOT(userChangedRuns()));
  connect(m_ui.rbCoAdd, SIGNAL(toggled(bool)), this,
          SLOT(fitTypeChanged(bool)));
  connect(m_ui.txtStart, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.txtEnd, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.chkCombine, SIGNAL(stateChanged(int)), this,
          SLOT(periodCombinationStateChanged(int)));
  connect(m_ui.txtSimFitLabel, SIGNAL(editingFinished()), this,
          SIGNAL(simulLabelChanged()));
  connect(this, SIGNAL(selectedGroupsChanged()), this,
          SLOT(checkForMultiGroupPeriodSelection()));
  connect(this, SIGNAL(selectedPeriodsChanged()), this,
          SLOT(checkForMultiGroupPeriodSelection()));
  connect(this, SIGNAL(workspaceChanged()), this,
          SLOT(checkForMultiGroupPeriodSelection()));
  connect(m_ui.cbDataset, SIGNAL(currentIndexChanged(int)), this,
          SIGNAL(datasetIndexChanged(int)));
  connect(m_ui.btnNextDataset, SIGNAL(clicked()), this, SLOT(setNextDataset()));
  connect(m_ui.btnPrevDataset, SIGNAL(clicked()), this,
          SLOT(setPreviousDataset()));
  connect(m_ui.txtFirst, SIGNAL(editingFinished()), this,
          SIGNAL(selectedPeriodsChanged()));
  connect(m_ui.txtSecond, SIGNAL(editingFinished()), this,
          SIGNAL(selectedPeriodsChanged()));
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
    // if buttons are disabled, enable them
    m_ui.rbCoAdd->setEnabled(true);
    m_ui.rbSimultaneous->setEnabled(true);
  } else {
    setFitType(FitType::Single);
  }
  emit workspaceChanged();
}

/**
 * Sets group names and updates checkboxes on UI
 * By default sets all unchecked
 * @param groups :: [input] List of group names
 */
void MuonFitDataSelector::setAvailableGroups(const QStringList &groups) {
  // If it's the same list, do nothing
  if (groups.size() == m_groupBoxes.size()) {
    auto existingGroups = m_groupBoxes.keys();
    auto newGroups = groups;
    qSort(existingGroups);
    qSort(newGroups);
    if (existingGroups == newGroups) {
      return;
    }
  }

  clearGroupCheckboxes();
  for (const auto group : groups) {
    addGroupCheckbox(group);
  }
}

/**
 * Get the user's supplied start time (default 0)
 * @returns :: start time input by user in microseconds
 */
double MuonFitDataSelector::getStartTime() const {
  // Validator ensures cast to double will succeed
  const QString start = m_ui.txtStart->text();
  return start.toDouble();
}

/**
 * Set the start time in the UI WITHOUT sending signal
 * @param start :: [input] Start time in microseconds
 */
void MuonFitDataSelector::setStartTimeQuietly(double start) {
  m_ui.txtStart->setText(QString::number(start));
}

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
double MuonFitDataSelector::getEndTime() const {
  // Validator ensures cast to double will succeed
  const QString end = m_ui.txtEnd->text();
  return end.toDouble();
}

/**
 * Set the end time in the UI WITHOUT sending signal
 * @param end :: [input] End time in microseconds
 */
void MuonFitDataSelector::setEndTimeQuietly(double end) {
  m_ui.txtEnd->setText(QString::number(end));
}

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
QStringList MuonFitDataSelector::getFilenames() const {
  return m_ui.runs->getFilenames();
}

/**
 * Set up input validation on UI controls
 * e.g. some boxes should only accept numeric input
 */
void MuonFitDataSelector::setUpValidators() {
  // Start/end times: numeric values only
  m_ui.txtStart->setValidator(new QDoubleValidator(this));
  m_ui.txtEnd->setValidator(new QDoubleValidator(this));
}

/**
 * Set up run finder with initial run number and instrument
 * @param runNumbers :: [input] Run numbers from loaded workspace
 * @param instName :: [input] Instrument name from loaded workspace
 */
void MuonFitDataSelector::setWorkspaceDetails(const QString &runNumbers,
                                              const QString &instName) {
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
  m_ui.runs->setFileTextWithSearch(runs);
}

/**
 * Set default values in some input controls
 * Defaults copy those previously used in muon fit property browser
 */
void MuonFitDataSelector::setDefaultValues() {
  m_ui.lblStart->setText(QString("Start (%1s)").arg(QChar(0x03BC)));
  m_ui.lblEnd->setText(QString("End (%1s)").arg(QChar(0x03BC)));
  this->setStartTime(0.0);
  this->setEndTime(0.0);
  setPeriodCombination(false);
  m_ui.txtSimFitLabel->setText("0");
  emit simulLabelChanged(); // make sure default "0" is set
}

/**
 * Set visibility of the "Periods" group box
 * (if single-period, hide to not confuse the user)
 * @param visible :: [input] Whether to show or hide the options
 */
void MuonFitDataSelector::setPeriodVisibility(bool visible) {
  m_ui.groupBoxPeriods->setVisible(visible);
}

/**
 * Add a new checkbox to the list of groups with given name
 * The new checkbox is unchecked by default
 * @param name :: [input] Name of group to add
 */
void MuonFitDataSelector::addGroupCheckbox(const QString &name) {
  auto checkBox = new QCheckBox(name);
  m_groupBoxes.insert(name, checkBox);
  checkBox->setChecked(false);
  m_ui.verticalLayoutGroups->addWidget(checkBox);
  connect(checkBox, SIGNAL(clicked(bool)), this,
          SIGNAL(selectedGroupsChanged()));
}

/**
 * Clears all group names and checkboxes
 * (ready to add new ones)
 */
void MuonFitDataSelector::clearGroupCheckboxes() {
  for (const auto &checkbox : m_groupBoxes) {
    m_ui.verticalLayoutGroups->removeWidget(checkbox);
    checkbox->deleteLater(); // will disconnect signal automatically
  }
  m_groupBoxes.clear();
}

/**
 * Sets checkboxes on UI for given number
 * of periods plus "combination" boxes.
 * Hides control for single-period data.
 * @param numPeriods :: [input] Number of periods
 */
void MuonFitDataSelector::setNumPeriods(size_t numPeriods) {
  const size_t currentPeriods = static_cast<size_t>(m_periodBoxes.size());
  if (numPeriods > currentPeriods) {
    // create more boxes
    for (size_t i = currentPeriods; i != numPeriods; i++) {
      QString name = QString::number(i + 1);
      auto checkbox = new QCheckBox(name);
      m_periodBoxes.insert(name, checkbox);
      m_ui.verticalLayoutPeriods->addWidget(checkbox);
    }
  } else if (numPeriods < currentPeriods) {
    // delete the excess
    QStringList toRemove;
    for (const QString name : m_periodBoxes.keys()) {
      const size_t periodNum = static_cast<size_t>(name.toInt());
      if (periodNum > numPeriods) {
        m_ui.verticalLayoutPeriods->removeWidget(m_periodBoxes.value(name));
        m_periodBoxes.value(name)->deleteLater(); // will disconnect signal
        toRemove.append(name);
      }
    }
    for (const QString name : toRemove) {
      m_periodBoxes.remove(name);
    }
  }

  // Ensure signals connected
  for (const auto &checkbox : m_periodBoxes) {
    connect(checkbox, SIGNAL(clicked()), this,
            SIGNAL(selectedPeriodsChanged()));
  }

  // Always put the combination at the bottom ("-1" = at end)
  m_ui.verticalLayoutPeriods->removeItem(m_ui.horizontalLayoutPeriodsCombine);
  m_ui.verticalLayoutPeriods->insertLayout(-1,
                                           m_ui.horizontalLayoutPeriodsCombine);

  // Hide box if single-period
  this->setPeriodVisibility(numPeriods > 1);
}

/**
 * Returns a list of periods and combinations chosen in UI
 * @returns :: list of periods e.g. "1", "3", "1+2-3+4", or "" if single-period
 */
QStringList MuonFitDataSelector::getPeriodSelections() const {
  QStringList checked;
  if (m_ui.groupBoxPeriods->isVisible()) {
    for (auto iter = m_periodBoxes.constBegin();
         iter != m_periodBoxes.constEnd(); ++iter) {
      if (iter.value()->isChecked()) {
        checked.append(iter.key());
      }
    }

    // combination
    if (m_ui.chkCombine->isChecked()) {
      QString combination = m_ui.txtFirst->text();
      const auto second = m_ui.txtSecond->text();
      if (!second.isEmpty()) {
        combination.append("-").append(m_ui.txtSecond->text());
      }
      combination.replace(" ", "");
      combination.replace(",", "+");
      checked.append(combination);
    }
  } else {
    // Single-period data
    checked << "";
  }
  return checked;
}

/**
 * Returns a list of the selected groups (checked boxes)
 * @returns :: list of selected groups
 */
QStringList MuonFitDataSelector::getChosenGroups() const {
  QStringList chosen;
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    if (iter.value()->isChecked()) {
      chosen.append(iter.key());
    }
  }
  return chosen;
}

/**
 * Set the chosen group ticked and all others off
 * Used when switching from Home tab to Data Analysis tab
 * @param group :: [input] Name of group to select
 */
void MuonFitDataSelector::setChosenGroup(const QString &group) {
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    if (iter.key() == group) {
      iter.value()->setChecked(true);
    }
  }
}

/**
 * Set the chosen period/combination ticked and all others off
 * Used when switching from Home tab to Data Analysis tab
 * @param period :: [input] Period string to set selected
 * (can be just one period or a combination)
 */
void MuonFitDataSelector::setChosenPeriod(const QString &period) {
  // Begin by unchecking everything
  for (auto checkbox : m_periodBoxes) {
    checkbox->setChecked(false);
  }

  // If single-period or all periods, string will be empty
  if (period.isEmpty()) {
    if (m_periodBoxes.size() == 1) { // single-period
      setPeriodCombination(false);
      m_periodBoxes.begin().value()->setChecked(true);
    } else { // all periods selected
      setPeriodCombination(true);
      QString combination;
      for (int i = 0; i < m_periodBoxes.count() - 1; i++) {
        combination.append(QString::number(i + 1)).append(", ");
      }
      m_ui.txtFirst->setText(
          combination.append(QString::number(m_periodBoxes.count())));
      m_ui.txtSecond->clear();
    }
  } else {
    // Test if period can be cast to int (just one period) or if it's a
    // combination e.g. "1+2"
    bool onePeriod(false);
    /*const int chosenPeriod = */ period.toInt(&onePeriod);
    if (onePeriod) {
      // set just one
      for (auto iter = m_periodBoxes.constBegin();
           iter != m_periodBoxes.constEnd(); ++iter) {
        if (iter.key() == period) {
          iter.value()->setChecked(true);
        }
      }
      setPeriodCombination(false);
    } else {
      // set the combination
      QStringList parts = period.split('-');
      if (parts.size() == 2) {
        m_ui.txtFirst->setText(parts[0].replace("+", ", "));
        m_ui.txtSecond->setText(parts[1].replace("+", ", "));
        setPeriodCombination(true);
      }
    }
  }
}

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
  if (!m_ui.rbCoAdd->isEnabled()) {
    const auto groups = getChosenGroups();
    const auto periods = getPeriodSelections();
    return groups.size() <= 1 && periods.size() <= 1 ? FitType::Single
                                                     : FitType::Simultaneous;
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
    m_ui.rbCoAdd->setEnabled(false);
    m_ui.rbSimultaneous->setEnabled(false);
  } else {
    m_ui.rbCoAdd->setEnabled(true);
    m_ui.rbSimultaneous->setEnabled(true);
    m_ui.rbCoAdd->setChecked(type == FitType::CoAdd);
    m_ui.rbSimultaneous->setChecked(type == FitType::Simultaneous);
  }
  checkForMultiGroupPeriodSelection();
}

/**
 * Check/uncheck period combination checkbox and set the textboxes
 * enabled/disabled
 * @param on :: [input] Turn on or off
 */
void MuonFitDataSelector::setPeriodCombination(bool on) {
  m_ui.chkCombine->setChecked(on);
  m_ui.txtFirst->setEnabled(on);
  m_ui.txtSecond->setEnabled(on);
}

/**
 * Slot: Keeps enabled/disabled state of textboxes in sync with checkbox
 * for period combination choices
 * @param state :: [input] New check state of box
 */
void MuonFitDataSelector::periodCombinationStateChanged(int state) {
  m_ui.txtFirst->setEnabled(state == Qt::Checked);
  m_ui.txtSecond->setEnabled(state == Qt::Checked);
  // If no text is set in the boxes, put something in there
  if (m_ui.txtFirst->text().isEmpty() && m_ui.txtSecond->text().isEmpty()) {
    m_ui.txtFirst->setText("1");
  }
  emit selectedPeriodsChanged();
}

/**
 * Return the instrument name currently set as the override
 * for the data selector
 * @returns :: instrument name
 */
QString MuonFitDataSelector::getInstrumentName() const {
  return m_ui.runs->getInstrumentOverride();
}

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
  disconnect(m_ui.runs, SIGNAL(fileInspectionFinished()), this,
             SLOT(unsetBusyState()));
  this->setCursor(Qt::ArrowCursor);
  m_ui.groupBoxDataSelector->setEnabled(true);
  m_ui.groupBoxGroups->setEnabled(true);
  if (m_ui.groupBoxPeriods->isVisible()) {
    m_ui.groupBoxPeriods->setEnabled(true);
  }
}

/**
 * Sets busy cursor and disables input while file search in progress.
 * Connects up slot to reset busy state when search done.
 */
void MuonFitDataSelector::setBusyState() {
  connect(m_ui.runs, SIGNAL(fileInspectionFinished()), this,
          SLOT(unsetBusyState()));
  this->setCursor(Qt::WaitCursor);
  m_ui.groupBoxDataSelector->setEnabled(false);
  m_ui.groupBoxGroups->setEnabled(false);
  if (m_ui.groupBoxPeriods->isVisible()) {
    m_ui.groupBoxPeriods->setEnabled(false);
  }
}

/**
 * Get text entered as the simultaneous fit label
 * @returns :: text entered in the textbox
 */
QString MuonFitDataSelector::getSimultaneousFitLabel() const {
  return m_ui.txtSimFitLabel->text();
}

/**
 * Set text of simultaneous fit label
 * @param label :: [input] Text to set as label
 */
void MuonFitDataSelector::setSimultaneousFitLabel(const QString &label) {
  m_ui.txtSimFitLabel->setText(label);
}

/**
 * Enable the "Label" textbox if multiple groups or periods are selected.
 * Called when groups/periods selection changes.
 */
void MuonFitDataSelector::checkForMultiGroupPeriodSelection() {
  const auto groups = getChosenGroups();
  const auto periods = getPeriodSelections();
  m_ui.txtSimFitLabel->setEnabled(groups.size() > 1 || periods.size() > 1 ||
                                  getFitType() == FitType::Simultaneous);
}

/**
 * Return index of currently selected dataset
 * @returns :: index of dataset selected in combobox
 */
int MuonFitDataSelector::getDatasetIndex() const {
  return m_ui.cbDataset->currentIndex();
}

/**
 * Return name of currently selected dataset
 * @returns :: name of dataset selected in combobox
 */
QString MuonFitDataSelector::getDatasetName() const {
  return m_ui.cbDataset->currentText();
}

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
  const int choice = QMessageBox::question(
      this, tr("MantidPlot - Overwrite Warning"),
      getSimultaneousFitLabel() +
          tr(" already exists. Do you want to replace it?"),
      QMessageBox::Yes | QMessageBox::Default,
      QMessageBox::No | QMessageBox::Escape);

  return choice == QMessageBox::Yes;
}

} // namespace MantidWidgets
} // namespace MantidQt
