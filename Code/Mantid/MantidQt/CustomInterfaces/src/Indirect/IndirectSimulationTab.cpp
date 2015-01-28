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

  }
} // namespace MantidQt
