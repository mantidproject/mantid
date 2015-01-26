#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectToolsTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectToolsTab::IndirectToolsTab(QWidget * parent) : IndirectTab(parent)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IndirectToolsTab::~IndirectToolsTab()
    {
    }

    /**
     * Method to build a URL to the appropriate page on the wiki for this tab.
     *
     * @return The URL to the wiki page
     */
    QString IndirectToolsTab::tabHelpURL()
    {
      return "http://www.mantidproject.org/IndirectTools:" + help();
    }

    /**
     * Emits a signal to run a python script using the method in the parent
     * UserSubWindow
     *
     * @param pyInput :: A string of python code to execute
     */
    void IndirectToolsTab::runPythonScript(const QString& pyInput)
    {
      emit executePythonScript(pyInput, false);
    }
  }
} // namespace MantidQt
