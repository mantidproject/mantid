#include "MantidQtCustomInterfaces/Quasi.h"
#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		Quasi::Quasi(QWidget * parent) : 
			IndirectBayesTab(parent), m_plot(new QwtPlot(parent)),
			m_propTree(new QtTreePropertyBrowser()), m_properties(), m_dblManager(new QtDoublePropertyManager())
		{
			m_uiForm.setupUi(parent);

			//add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);

			m_properties["EMin"] = m_dblManager->addProperty("EMin");
			m_properties["EMax"] = m_dblManager->addProperty("EMax");
			m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
			m_properties["ResBinning"] = m_dblManager->addProperty("Resolution Binning");
			
			m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["SampleBinning"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["ResBinning"], NUM_DECIMALS);

			m_propTree->addProperty(m_properties["EMin"]);
			m_propTree->addProperty(m_properties["EMax"]);
			m_propTree->addProperty(m_properties["SampleBinning"]);
			m_propTree->addProperty(m_properties["ResBinning"]);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			m_plot->setCanvasBackground(Qt::white);
   		m_plot->setAxisFont(QwtPlot::xBottom, parent->font());
    	m_plot->setAxisFont(QwtPlot::yLeft, parent->font());
		}

		void Quasi::validate()
		{

		}

		void Quasi::run() 
		{

		}

		void Quasi::help()
		{

		}
	} // namespace CustomInterfaces
} // namespace MantidQt
