#include "MantidSurfacePlotDialog.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ExperimentInfo.h"

using Mantid::API::IMDWorkspace;
using Mantid::API::IMDWorkspace_sptr;
using Mantid::API::ExperimentInfo;

/**
 * Construct an object of this type
 * @param mui :: The MantidUI area
 * @param flags :: Window flags that are passed the the QDialog constructor
 * @param wsNames :: the names of the workspaces to be plotted
 */
MantidSurfacePlotDialog::MantidSurfacePlotDialog(MantidUI *mui,
                                                 Qt::WFlags flags,
                                                 QList<QString> wsNames)
    : QDialog(mui->appWindow(), flags), m_mantidUI(mui), m_wsNames(wsNames),
      m_accepted(false), m_widget(this, flags, wsNames, false) {
  // Set up UI.
  init();
}

/**
 * Set up layout of dialog
 */
void MantidSurfacePlotDialog::init() {
  m_outer = new QVBoxLayout();
  setWindowTitle(tr("Surface plot versus log value"));
  m_outer->insertWidget(1, &m_widget);
  initLogs();
  initButtons();
  setLayout(m_outer);
}

/**
 * Set up UI to choose a log and name of axis
 */
void MantidSurfacePlotDialog::initLogs() {

  m_logBox = new QVBoxLayout;
  m_logLabel = new QLabel(tr("Log value to plot against:"));
  m_logSelector = new QComboBox();
  populateLogComboBox();
  m_axisLabel = new QLabel(tr("<br>Label for plot axis:"));
  m_axisNameEdit = new QLineEdit();

  m_logBox->add(m_logLabel);
  m_logBox->add(m_logSelector);
  m_logBox->add(m_axisLabel);
  m_logBox->add(m_axisNameEdit);
  m_outer->addItem(m_logBox);
}

/**
 * Set up buttons on UI (OK/Cancel)
 */
void MantidSurfacePlotDialog::initButtons() {
  m_buttonBox = new QHBoxLayout;

  m_okButton = new QPushButton("OK");
  m_cancelButton = new QPushButton("Cancel");

  m_buttonBox->addWidget(m_okButton);
  m_buttonBox->addWidget(m_cancelButton);

  m_outer->addItem(m_buttonBox);

  connect(m_okButton, SIGNAL(clicked()), this, SLOT(plot()));
  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(close()));
}

/**
 * Populate the log combo box with all log names that
 * have single numeric value per workspace
 */
void MantidSurfacePlotDialog::populateLogComboBox() {
  // First item should be "Workspace index"
  m_logSelector->addItem(tr("Workspace index"));

  // We have been given a list of names of MatrixWorkspaces (m_wsNames)
  // Get the log names out of all of them
  std::set<std::string> logNames;
  for (auto wsName : m_wsNames) {
    auto ws = m_mantidUI->getWorkspace(wsName);
    if (ws) {
      // It should be MatrixWorkspace, which is an ExperimentInfo
      auto ei = boost::dynamic_pointer_cast<const ExperimentInfo>(ws);
      if (ei) {
        const std::vector<Mantid::Kernel::Property *> &logData =
            ei->run().getLogData();
        for (auto log : logData) {
          // If this is a single-value numeric log, add it to the list
          if (dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(log) ||
              dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(log)) {
            logNames.insert(log->name());
          }
        }
      }
    }
  }
  // Add the log names to the combo box
  for (std::string name : logNames) {
    m_logSelector->addItem(name.c_str());
  }
}

/**
 * Gets the log that user selected to plot against
 * @returns Name of log, or "Workspace index"
 */
const QString MantidSurfacePlotDialog::getLogName() const {
  return m_logSelector->currentText();
}

/**
 * Gets the name that the user gave for the Y axis of the surface plot
 * @returns Name input by user for axis
 */
const QString MantidSurfacePlotDialog::getAxisName() const {
  return m_axisNameEdit->text();
}

/**
 * Returns a structure holding all of the selected options.
 * @returns Struct holding user input
 */
MantidSurfacePlotDialog::UserInputSurface
MantidSurfacePlotDialog::getSelections() const {
  UserInputSurface selections;
  selections.accepted = m_accepted;
  selections.plotIndex = getPlot();
  selections.axisName = getAxisName();
  selections.logName = getLogName();
  return selections;
}

/**
* Returns the workspace index to be plotted
* @returns Workspace index to be plotted
*/
const int MantidSurfacePlotDialog::getPlot() const {
  int spectrumIndex{0}; // default to 0
  const auto userInput = m_widget.getPlots();

  if (!userInput.empty()) {
    const auto indexList = userInput.values();
    if (!indexList.empty()) {
      const auto spectrumIndexes = indexList.at(0);
      if (!spectrumIndexes.empty()) {
        spectrumIndex = *spectrumIndexes.begin();
      }
    }
  }
  return spectrumIndex;
}

/**
 * Called when OK button pressed
 */
void MantidSurfacePlotDialog::plot() {
  if (m_widget.plotRequested()) {
    m_accepted = true;
    accept();
  }
}
