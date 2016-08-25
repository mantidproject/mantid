#include "MantidQtMantidWidgets/LogValueSelector.h"

using Mantid::Kernel::Math::StatisticType;

namespace MantidQt {
namespace MantidWidgets {

// Convert strings to enum values
const std::map<std::string, StatisticType> LogValueSelector::STRING_TO_FUNC = {
    {"Mean", StatisticType::Mean},
    {"Min", StatisticType::Minimum},
    {"Max", StatisticType::Maximum},
    {"First", StatisticType::FirstValue},
    {"Last", StatisticType::LastValue}};

/**
 * Constructor for the widget
 * @param parent :: [input] Parent dialog for the widget
 */
LogValueSelector::LogValueSelector(QWidget *parent)
    : API::MantidWidget(parent) {
  m_ui.setupUi(this);
  m_ui.horizontalLayout->addStretch(1);
  doConnect();
  m_ui.chkUseLog->setChecked(false);
  this->setEnabled(Qt::Unchecked);
}

/**
 * Set up signal/slot connections
 */
void LogValueSelector::doConnect() {
  connect(m_ui.chkUseLog, SIGNAL(stateChanged(int)), this,
          SLOT(setEnabled(int)));
}

/**
 * Get selected log text
 * @returns Text selected in log dropdown
 */
QString LogValueSelector::getLog() const { return m_ui.log->currentText(); }

/**
 * Get selected function text
 * @returns Text selected in function dropdown
 */
QString LogValueSelector::getFunctionText() const {
  return m_ui.function->currentText();
}

/**
 * Get selected function
 * @returns Function selected in dropdown
 */
StatisticType LogValueSelector::getFunction() const {
  const auto &text = getFunctionText().toStdString();
  return STRING_TO_FUNC.at(text);
}

/**
 * Whether checkbox is shown or not
 * @returns whether checkbox is visible
 */
bool LogValueSelector::isCheckboxShown() const {
  return m_ui.chkUseLog->isVisible();
}

/**
 * Control whether checkbox is shown
 * @param visible :: [input] Whether checkbox should be visible
 */
void LogValueSelector::setCheckboxShown(bool visible) {
  m_ui.chkUseLog->setVisible(visible);
}

/**
 * Get a pointer to log combo box
 * @returns :: Pointer to log combo box
 */
QComboBox *LogValueSelector::getLogComboBox() const { return m_ui.log; }

/**
 * Slot: set enabled/disabled
 * @param checkstate :: [input] State of checkbox
 */
void LogValueSelector::setEnabled(int checkstate) {
  const bool enabled = checkstate == Qt::Checked;
  m_ui.log->setEnabled(enabled);
  m_ui.function->setEnabled(enabled);
  emit logOptionsEnabled(enabled);
}

/**
 * Returns whether checkbox is ticked or not
 * @returns :: true if checkbox is ticked
 */
bool LogValueSelector::isCheckboxTicked() const {
  return m_ui.chkUseLog->isChecked();
}

} // namespace MantidWidgets
} // namespace MantidQt
