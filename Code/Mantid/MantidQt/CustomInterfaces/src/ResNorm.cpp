#include "MantidQtCustomInterfaces/ResNorm.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		ResNorm::ResNorm(QWidget * parent) : 
			IndirectBayesTab(parent),	m_intManager(new QtIntPropertyManager())
		{
			m_uiForm.setupUi(parent);

			//add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);
			m_properties["EMin"] = m_dblManager->addProperty("EMin");
			m_properties["EMax"] = m_dblManager->addProperty("EMax");
			m_properties["VanBinning"] = m_intManager->addProperty("Van Binning");
			
			m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

			m_propTree->addProperty(m_properties["EMin"]);
			m_propTree->addProperty(m_properties["EMax"]);
			m_propTree->addProperty(m_properties["VanBinning"]);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			m_plot->setCanvasBackground(Qt::white);
   		m_plot->setAxisFont(QwtPlot::xBottom, parent->font());
    	m_plot->setAxisFont(QwtPlot::yLeft, parent->font());
		}

		void ResNorm::validate()
		{

		}

		void ResNorm::run() 
		{

		}

		void ResNorm::help()
		{

		}
	} // namespace CustomInterfaces
} // namespace MantidQt
