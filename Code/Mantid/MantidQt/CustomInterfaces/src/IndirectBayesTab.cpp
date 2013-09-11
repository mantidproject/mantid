#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectBayesTab::IndirectBayesTab(QWidget * parent) : QWidget(parent),  
      m_plot(new QwtPlot(parent)), m_propTree(new QtTreePropertyBrowser()), 
      m_properties(), m_dblManager(new QtDoublePropertyManager())
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IndirectBayesTab::~IndirectBayesTab()
    {
    }
  }
} // namespace MantidQt
