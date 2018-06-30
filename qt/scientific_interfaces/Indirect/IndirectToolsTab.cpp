#include "IndirectToolsTab.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectToolsTab::IndirectToolsTab(QWidget *parent) : IndirectTab(parent) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectToolsTab::~IndirectToolsTab() {}

/**
 * Emits a signal to run a python script using the method in the parent
 * UserSubWindow
 *
 * @param pyInput :: A string of python code to execute
 */
void IndirectToolsTab::runPythonScript(const QString &pyInput) {
  emit executePythonScript(pyInput, false);
}
} // namespace CustomInterfaces
} // namespace MantidQt
