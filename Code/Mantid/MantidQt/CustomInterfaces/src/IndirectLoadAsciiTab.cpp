#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectLoadAsciiTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectLoadAsciiTab::IndirectLoadAsciiTab(QWidget * parent) : QWidget(parent)
    {

    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IndirectLoadAsciiTab::~IndirectLoadAsciiTab()
    {
    }

    /**
     * Method to build a URL to the appropriate page on the wiki for this tab.
     * 
     * @return The URL to the wiki page
     */
    QString IndirectLoadAsciiTab::tabHelpURL()
    { 
      return "http://www.mantidproject.org/IndirectLoadASCII:" + help();
    }

    /**
     * Emits a signal to run a python script using the method in the parent
     * UserSubWindow
     * 
     * @param pyInput :: A string of python code to execute
     */
    void IndirectLoadAsciiTab::runPythonScript(const QString& pyInput)
    {
      emit executePythonScript(pyInput, false);
    }
  }
} // namespace MantidQt
