#include "MantidQtMantidWidgets/MuonFitDataSelector.h"
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

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
    : API::MantidWidget(parent) {
  m_ui.setupUi(this);
  this->setUpValidators();
  this->setDefaultValues();
  this->setWorkspaceDetails(runNumber, instName);
  this->setNumPeriods(numPeriods);
  this->setGroupingOptions(groups);
  // connect signals and slots here
};

/**
 * Gets user input in the form of a QVariant
 *
 * This is implemented as the "standard" way of getting input from a
 * MantidWidget. In practice it is probably easier to get the input
 * using other methods.
 *
 * The returned QVariant is a QVariantMap of (parameter, value) pairs.
 * @returns :: QVariant containing a QVariantMap
 */
QVariant MuonFitDataSelector::getUserInput() const {
  QVariant ret;
  auto map = ret.asMap();
  map.insert("Workspace index", getWorkspaceIndex());
  map.insert("Start", getStartTime());
  map.insert("End", getEndTime());
  map.insert("Runs", getRuns());
  throw std::runtime_error("TODO: implement this function");
}

/**
 * Sets user input in the form of a QVariant
 *
 * This is implemented as the "standard" way of setting input in a
 * MantidWidget. In practice it is probably easier to set the input
 * using other methods.
 *
 * The input QVariant is a QVariantMap of (parameter, value) pairs.
 * @param value :: [input] QVariant containing a QVariantMap
 */
void MuonFitDataSelector::setUserInput(const QVariant &value) {
  Q_UNUSED(value);
  throw std::runtime_error("TODO: implement this function");
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
 * Get the user's supplied start time (default 0)
 * @returns :: start time input by user in microseconds
 */
double MuonFitDataSelector::getStartTime() const {
  // Validator ensures cast to double will succeed
  const QString start = m_ui.txtStart->text();
  return start.toDouble();
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
 * Get the filenames of the supplied run numbers
 * @returns :: list of run filenames
 */
QStringList MuonFitDataSelector::getRuns() const {
  // Run file search in case anything's changed
  m_ui.runs->findFiles();
  // Wait for file search to finish.
  while (m_ui.runs->isSearching()) {
    QApplication::processEvents();
  }
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
 * @param runNumber :: [input] Run number from loaded workspace
 * @param instName :: [input] Instrument name from loaded workspace
 */
void MuonFitDataSelector::setWorkspaceDetails(int runNumber,
                                              const QString &instName) {
  m_startingRun = runNumber;
  m_instName = instName;
  // Set initial run to be run number of the workspace loaded in Home tab
  m_ui.runs->setText(QString::number(m_startingRun) + "-");
  // Set the file finder to the correct instrument (not Mantid's default)
  m_ui.runs->setInstrumentOverride(m_instName);
}

/**
 * Set default values in some input controls
 * Defaults copy those previously used in muon fit property browser
 */
void MuonFitDataSelector::setDefaultValues() {
  m_ui.lblStart->setText(QString::fromUtf8("Start (µs)"));
  m_ui.lblEnd->setText(QString::fromUtf8("End (µs)"));
  m_ui.txtWSIndex->setText("0");
  m_ui.txtStart->setText("0.0");
  m_ui.txtEnd->setText("10.0");
}

/**
 * Sets number of periods and updates checkboxes on UI
 * @param numPeriods :: [input] Number of periods in data
 */
void MuonFitDataSelector::setNumPeriods(size_t numPeriods) {
  m_numPeriods = numPeriods;
  // TODO: UI code here, add/remove checkboxes, hide if = 0
  throw std::runtime_error("TODO: implement this function");
}

/**
 * Sets group names and updates checkboxes on UI
 * @param groups :: [input] List of group names
 */
void MuonFitDataSelector::setGroupingOptions(const QStringList &groups) {
    // TODO: something here inc. UI code with checkboxes
    //m_ui.verticalLayoutGroups->count();
  throw std::runtime_error("TODO: implement this function");
}

} // namespace MantidWidgets
} // namespace MantidQt
