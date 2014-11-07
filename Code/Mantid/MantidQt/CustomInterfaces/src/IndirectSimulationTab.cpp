#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectSimulationTab.h"

using namespace Mantid::API;

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectSimulationTab::IndirectSimulationTab(QWidget * parent) : QWidget(parent),
      m_algoRunner(parent)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IndirectSimulationTab::~IndirectSimulationTab()
    {
    }

    /**
     * Method to build a URL to the appropriate page on the wiki for this tab.
     * 
     * @return The URL to the wiki page
     */
    QString IndirectSimulationTab::tabHelpURL()
    { 
      return "http://www.mantidproject.org/IndirectSimualtion:" + help();
    }

    /**
     * Emits a signal to run a python script using the method in the parent
     * UserSubWindow
     * 
     * @param pyInput :: A string of python code to execute
     */
    void IndirectSimulationTab::runPythonScript(const QString& pyInput)
    {
      emit executePythonScript(pyInput, false);
    }

    /**
     * Runs an algorithm async from the UI thread.
     *
     * @param alg Configured algorithm to run
     */
    void IndirectSimulationTab::runAlgorithm(const IAlgorithm_sptr alg)
    {
      m_algoRunner.startAlgorithm(alg);
    }

  }
} // namespace MantidQt
