#include "MantidQtCustomInterfaces/Stretch.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		Stretch::Stretch(QWidget * parent) : 
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

			//add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);

			m_properties["EMin"] = m_dblManager->addProperty("EMin");
			m_properties["EMax"] = m_dblManager->addProperty("EMax");
			m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
			m_properties["Sigma"] = m_dblManager->addProperty("Sigma");
			m_properties["Beta"] = m_dblManager->addProperty("Beta");
			
			m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
			m_dblManager->setDecimals(m_properties["Sigma"], INT_DECIMALS);
			m_dblManager->setDecimals(m_properties["Beta"], INT_DECIMALS);

			m_propTree->addProperty(m_properties["EMin"]);
			m_propTree->addProperty(m_properties["EMax"]);
			m_propTree->addProperty(m_properties["SampleBinning"]);
			m_propTree->addProperty(m_properties["Sigma"]);
			m_propTree->addProperty(m_properties["Beta"]);

			//default values
			m_dblManager->setValue(m_properties["Sigma"], 50);
			m_dblManager->setValue(m_properties["Beta"], 50);
			m_dblManager->setValue(m_properties["SampleBinning"], 1);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			m_plot->setCanvasBackground(Qt::white);
   		m_plot->setAxisFont(QwtPlot::xBottom, parent->font());
    	m_plot->setAxisFont(QwtPlot::yLeft, parent->font());
		}

		bool Stretch::validate()
		{
			return true;
		}

		void Stretch::run() 
		{

		}

		void Stretch::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["EMin"], min);
    }

    void Stretch::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["EMax"], max);	
    }

    void Stretch::updateProperties(QtProperty* prop, double val)
    {
    	if(prop == m_properties["EMin"])
    	{
    		m_rangeSelector->setMinimum(val);
    	}
    	else if (prop == m_properties["EMax"])
    	{
				m_rangeSelector->setMaximum(val);
    	}
    }

	} // namespace CustomInterfaces
} // namespace MantidQt
