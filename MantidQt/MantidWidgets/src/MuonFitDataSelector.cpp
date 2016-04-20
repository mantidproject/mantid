#include "MantidQtMantidWidgets/MuonFitDataSelector.h"

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
 * something" i.e. workspacePropertiesChanged, selectedGroupsChanged,
 * selectedPeriodsChanged
 */
void MuonFitDataSelector::setUpConnections() {
  connect(m_ui.runs, SIGNAL(filesFound()), this, SLOT(userChangedRuns()));
  connect(m_ui.txtWSIndex, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.txtStart, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.txtEnd, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.chkCombine, SIGNAL(stateChanged(int)), this,
          SIGNAL(selectedPeriodsChanged()));
}

/**
 * Slot: called when user edits runs box.
 * Check for single run and enable/disable radio buttons,
 * and emit signal that runs have changed.
 */
void MuonFitDataSelector::userChangedRuns() {
  // check for single run and enable/disable radio buttons
  const auto runs = getRuns();
  if (runs.size() < 2) {
    setFitType(FitType::Single);
  } else {
    // if buttons are disabled, enable them
    m_ui.rbCoAdd->setEnabled(true);
    m_ui.rbSimultaneous->setEnabled(true);
  }
  emit workspaceChanged();
}

/**
 * Sets group names and updates checkboxes on UI
 * By default sets all checked
 * @param groups :: [input] List of group names
 */
void MuonFitDataSelector::setAvailableGroups(const QStringList &groups) {
  clearGroupCheckboxes();
  for (const auto group : groups) {
    addGroupCheckbox(group);
    setGroupSelected(group, true);
  }
}

/**
 * Get the user's supplied workspace index (default 0)
 * Returns an unsigned int so it can be put into a QVariant
 * @returns :: Workspace index input by user
 */
unsigned int MuonFitDataSelector::getWorkspaceIndex() const {
  // Validator ensures this can be cast to a positive integer
  const QString index = m_ui.txtWSIndex->text();
  return index.toUInt();
}

/**
 * Set the workspace index in the UI
 * @param index :: [input] Workspace index to set
 */
void MuonFitDataSelector::setWorkspaceIndex(unsigned int index) {
  m_ui.txtWSIndex->setText(QString::number(index));
  emit dataPropertiesChanged();
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
QStringList MuonFitDataSelector::getRuns() const {
  return m_ui.runs->getFilenames();
}

/**
 * Set up input validation on UI controls
 * e.g. some boxes should only accept numeric input
 */
void MuonFitDataSelector::setUpValidators() {
  // WS index: non-negative integers only
  m_ui.txtWSIndex->setValidator(new QIntValidator(0, 1000, this));
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
  // Set initial run to be run number of the workspace loaded in Home tab
  QString runs(runNumbers);
  runs.remove(QRegExp("^[0]*")); // remove leading zeros, if any
  m_ui.runs->setText(runs);
  // Set fit type - co-add (as this comes from Home tab) or single
  if (runs.contains('-') || runs.contains(',')) {
    setFitType(FitType::CoAdd);
  } else {
    setFitType(FitType::Single);
  }
  // Set the file finder to the correct instrument (not Mantid's default)
  m_ui.runs->setInstrumentOverride(instName);
}

/**
 * Set default values in some input controls
 * Defaults copy those previously used in muon fit property browser
 */
void MuonFitDataSelector::setDefaultValues() {
  m_ui.lblStart->setText(QString("Start (%1s)").arg(QChar(0x03BC)));
  m_ui.lblEnd->setText(QString("End (%1s)").arg(QChar(0x03BC)));
  this->setWorkspaceIndex(0);
  this->setStartTime(0.0);
  this->setEndTime(0.0);
  m_ui.chkCombine->setChecked(false);
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
 * @param name :: [input] Name of group to add
 */
void MuonFitDataSelector::addGroupCheckbox(const QString &name) {
  auto checkBox = new QCheckBox(name);
  m_groupBoxes.insert(name, checkBox);
  m_ui.verticalLayoutGroups->add(checkBox);
  connect(checkBox, SIGNAL(stateChanged(int)), this,
          SIGNAL(selectedGroupsChanged()));
}

/**
 * Clears all group names and checkboxes
 * (ready to add new ones)
 */
void MuonFitDataSelector::clearGroupCheckboxes() {
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    m_ui.verticalLayoutGroups->remove(iter.data());
    iter.data()->deleteLater(); // will disconnect signal automatically
  }
  m_groupBoxes.clear();
}

/**
 * Set selection state of given group
 * @param name :: [input] Name of group to select/deselect
 * @param selected :: [input] True to select, false to deselect
 */
void MuonFitDataSelector::setGroupSelected(const QString &name, bool selected) {
  if (m_groupBoxes.contains(name)) {
    m_groupBoxes.value(name)->setChecked(selected);
    emit selectedGroupsChanged();
  } else {
    g_log.warning() << "No group called " << name.toStdString()
                    << ": cannot set selection state";
  }
}

/**
 * Sets checkboxes on UI for given number
 * of periods plus "combination" boxes.
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
      m_ui.verticalLayoutPeriods->add(checkbox);
      connect(checkbox, SIGNAL(stateChanged(int)), this,
              SIGNAL(selectedPeriodsChanged()));
    }
  } else if (numPeriods < currentPeriods) {
    // delete the excess
    QStringList toRemove;
    for (const QString name : m_periodBoxes.keys()) {
      const size_t periodNum = static_cast<size_t>(name.toInt());
      if (periodNum > numPeriods) {
        m_ui.verticalLayoutPeriods->remove(m_periodBoxes.value(name));
        m_periodBoxes.value(name)->deleteLater(); // will disconnect signal
        toRemove.append(name);
      }
    }
    for (const QString name : toRemove) {
      m_periodBoxes.remove(name);
    }
  }
  this->setPeriodVisibility(numPeriods > 1);
}

/**
 * Returns a list of periods and combinations chosen in UI
 * @returns :: list of periods e.g. "1", "3", "1+2-3+4"
 */
QStringList MuonFitDataSelector::getPeriodSelections() const {
  QStringList checked;
  for (auto iter = m_periodBoxes.constBegin(); iter != m_periodBoxes.constEnd();
       ++iter) {
    if (iter.value()->isChecked()) {
      checked.append(iter.key());
    }
  }

  // combination
  if (m_ui.chkCombine->isChecked()) {
    QString combination = m_ui.txtFirst->text();
    combination.append("-").append(m_ui.txtSecond->text());
    combination.replace(" ", "");
    combination.replace(",", "+");
    checked.append(combination);
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
  QVariant ret;
  auto map = ret.asMap();
  map.insert("Workspace index", getWorkspaceIndex());
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
 * This function doesn't support setting runs, chosen groups or chosen periods
 * (done through UI only).
 *
 * The input QVariant is a QVariantMap of (parameter, value) pairs.
 * @param value :: [input] QVariant containing a QVariantMap
 */
void MuonFitDataSelector::setUserInput(const QVariant &value) {
  if (value.canConvert(QVariant::Map)) {
    const auto map = value.toMap();
    if (map.contains("Workspace index")) {
      setWorkspaceIndex(map.value("Workspace index").toUInt());
    }
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
 * - If only one run is selected, this is a single fit.
 * - If multiple runs are selected, the user has the option of co-adding them or
 * doing a simultaneous fit, chosen via the radio buttons.
 * @returns :: fit type from enum
 */
IMuonFitDataSelector::FitType MuonFitDataSelector::getFitType() const {
  // If radio buttons disabled, it's a single fit
  if (!m_ui.rbCoAdd->isEnabled()) {
    return FitType::Single;
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
}

} // namespace MantidWidgets
} // namespace MantidQt
