#include "MantidQtCustomInterfaces/JumpFit.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		JumpFit::JumpFit(QWidget * parent) : 
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

			//add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);

			m_properties["QMin"] = m_dblManager->addProperty("QMin");
			m_properties["QMax"] = m_dblManager->addProperty("QMax");
			
			m_dblManager->setDecimals(m_properties["QMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["QMax"], NUM_DECIMALS);

			m_propTree->addProperty(m_properties["QMin"]);
			m_propTree->addProperty(m_properties["QMax"]);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			m_plot->setCanvasBackground(Qt::white);
   		m_plot->setAxisFont(QwtPlot::xBottom, parent->font());
    	m_plot->setAxisFont(QwtPlot::yLeft, parent->font());
		}

		bool JumpFit::validate()
		{
			return true;
		}

		void JumpFit::run() 
		{

		}

		void JumpFit::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["QMin"], min);
    }

    void JumpFit::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["QMax"], max);	
    }

    void JumpFit::updateProperties(QtProperty* prop, double val)
    {
    	if(prop == m_properties["QMin"])
    	{
    		m_rangeSelector->setMinimum(val);
    	}
    	else if (prop == m_properties["QMax"])
    	{
				m_rangeSelector->setMaximum(val);
    	}
    }
	} // namespace CustomInterfaces
} // namespace MantidQt
