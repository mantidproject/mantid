#include "MantidSurfacePlotDialog.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ExperimentInfo.h"
#include <QMessageBox>

using Mantid::API::IMDWorkspace;
using Mantid::API::IMDWorkspace_sptr;
using Mantid::API::ExperimentInfo;

/// The string "Workspace index"
const QString MantidSurfacePlotDialog::WORKSPACE_INDEX = "Workspace index";

/// The string "Custom"
const QString MantidSurfacePlotDialog::CUSTOM = "Custom";

/// Minimum width for dialog to fit the title string in
const int MantidSurfacePlotDialog::MINIMUM_WIDTH = 275;

/**
 * Construct an object of this type
 * @param mui :: The MantidUI area
 * @param flags :: Window flags that are passed the the QDialog constructor
 * @param wsNames :: the names of the workspaces to be plotted
 * @param plotType :: Type of plot (for window title)
 */
MantidSurfacePlotDialog::MantidSurfacePlotDialog(MantidUI *mui,
                                                 Qt::WFlags flags,
                                                 QList<QString> wsNames,
                                                 const QString &plotType)
    : QDialog(mui->appWindow(), flags), m_widget(this, flags, wsNames, false),
      m_mantidUI(mui), m_wsNames(wsNames), m_accepted(false) {
  // Set up UI.
  init(plotType);
}

/**
 * Set up layout of dialog
 * @param plotType :: Type of plot (for window title)
 */
void MantidSurfacePlotDialog::init(const QString &plotType) {
  m_outer = new QVBoxLayout();
  QString title(plotType);
  title.append(tr(" plot versus log value"));
  setWindowTitle(title);
  m_outer->insertWidget(1, &m_widget);
  initLogs();
  initButtons();
  setLayout(m_outer);
  this->setMinimumWidth(MINIMUM_WIDTH);
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
  m_axisNameEdit->setText(m_logSelector->currentText());
  m_customLogLabel = new QLabel(tr("<br>Custom log values:"));
  m_logValues = new QLineEdit();

  m_logBox->add(m_logLabel);
  m_logBox->add(m_logSelector);
  m_logBox->add(m_customLogLabel);
  m_logBox->add(m_logValues);
  m_logBox->add(m_axisLabel);
  m_logBox->add(m_axisNameEdit);
  m_outer->addItem(m_logBox);

  m_logValues->setEnabled(false);

  connect(m_logSelector, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(onLogSelected(const QString &)));
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
 * have single numeric value per workspace (and occur
 * in every workspace)
 */
void MantidSurfacePlotDialog::populateLogComboBox() {
  // First item should be "Workspace index"
  m_logSelector->addItem(WORKSPACE_INDEX);

  // Create a table of all single-value numeric log names versus
  // how many workspaces they appear in
  std::map<std::string, int> logCounts;
  for (auto wsName : m_wsNames) {
    auto ws = m_mantidUI->getWorkspace(wsName);
    if (ws) {
      // It should be MatrixWorkspace, which is an ExperimentInfo
      auto ei = boost::dynamic_pointer_cast<const ExperimentInfo>(ws);
      if (ei) {
        const std::vector<Mantid::Kernel::Property *> &logData =
            ei->run().getLogData();
        for (auto log : logData) {
          // If this is a single-value numeric log, add it to the list of counts
          if (dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(log) ||
              dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(log)) {
            const std::string name = log->name();
            if (logCounts.find(name) != logCounts.end()) {
              logCounts[name]++;
            } else {
              logCounts[name] = 1;
            }
          }
        }
      }
    }
  }

  // Add the log names to the combo box if they appear in all workspaces
  const int nWorkspaces = m_wsNames.size();
  for (auto logCount : logCounts) {
    if (logCount.second == nWorkspaces) {
      m_logSelector->addItem(logCount.first.c_str());
    }
  }

  // Add "Custom" at the end of the list
  m_logSelector->addItem(CUSTOM);
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
  if (selections.logName == CUSTOM) {
    try {
      selections.customLogValues = getCustomLogValues();
    } catch (const std::invalid_argument &ex) {
      QString error("Invalid log value supplied: ");
      showPlotOptionsError(error.append(ex.what()));
      selections.accepted = false;
    }
  }
  return selections;
}

/**
* Returns the workspace index to be plotted
* @returns Workspace index to be plotted
*/
int MantidSurfacePlotDialog::getPlot() const {
  int spectrumIndex = 0; // default to 0
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

/**
 * Called when log selection changed
 * If "Custom" selected, enable the custom log input box.
 * Otherwise, it is read-only.
 * Also put the log name into the axis name box as a default choice.
 * @param logName :: [input] Text selected in combo box
 */
void MantidSurfacePlotDialog::onLogSelected(const QString &logName) {
  m_logValues->setEnabled(logName == CUSTOM);
  m_logValues->clear();
  m_axisNameEdit->setText(logName);
}

/**
 * If "Custom" is selected as log, returns the list of values the user has input
 * into the edit box, otherwise returns an empty set.
 * Note that the set is ordered by definition, and values are only added if they
 * are successfully converted to a double.
 * @returns Set of numerical log values
 * @throws invalid_argument if values are not numeric
 */
const std::set<double> MantidSurfacePlotDialog::getCustomLogValues() const {
  std::set<double> logValues;
  if (m_logSelector->currentText() == CUSTOM) {
    QStringList values = m_logValues->text().split(',');
    foreach (QString value, values) {
      bool ok = false;
      double number = value.toDouble(&ok);
      if (ok) {
        logValues.insert(number);
      } else {
        throw std::invalid_argument(value.toStdString());
      }
    }
  }
  return logValues;
}

/**
 * Displays a message box with the supplied error string.
 * @param message :: [input] Error message to display
 */
void MantidSurfacePlotDialog::showPlotOptionsError(const QString &message) {
  if (!message.isEmpty()) {
    QMessageBox errorMessage;
    errorMessage.setText(message);
    errorMessage.setIcon(QMessageBox::Critical);
    errorMessage.exec();
  }
}
