#include "MantidQtMantidWidgets/LogValueSelector.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor for the widget
 * @param parent :: [input] Parent dialog for the widget
 */
LogValueSelector::LogValueSelector(QWidget *parent)
    : API::MantidWidget(parent) {
  m_ui.setupUi(this);
  doConnect();
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
QString LogValueSelector::getFunction() const {
  return m_ui.function->currentText();
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
}

} // namespace MantidWidgets
} // namespace MantidQt
