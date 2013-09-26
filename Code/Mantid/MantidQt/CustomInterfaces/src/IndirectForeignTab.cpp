#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectForeignTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectForeignTab::IndirectForeignTab(QWidget * parent) : QWidget(parent)
    {

    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IndirectForeignTab::~IndirectForeignTab()
    {
    }

    /**
     * Method to build a URL to the appropriate page on the wiki for this tab.
     * 
     * @return The URL to the wiki page
     */
    QString IndirectForeignTab::tabHelpURL()
    { 
      return "http://www.mantidproject.org/IndirectBayes:" + help();
    }

    /**
     * Emits a signal to run a python script using the method in the parent
     * UserSubWindow
     * 
     * @param pyInput :: A string of python code to execute
     */
    void IndirectForeignTab::runPythonScript(const QString& pyInput)
    {
      emit executePythonScript(pyInput, false);
    }
  }
} // namespace MantidQt
