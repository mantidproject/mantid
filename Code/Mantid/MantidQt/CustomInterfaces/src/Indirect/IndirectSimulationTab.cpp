#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectSimulationTab.h"

using namespace Mantid::API;

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectSimulationTab::IndirectSimulationTab(QWidget * parent) : IndirectTab(parent)
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

  }
} // namespace MantidQt
