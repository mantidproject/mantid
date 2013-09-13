#include "MantidQtCustomInterfaces/Quasi.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		Quasi::Quasi(QWidget * parent) : 
			IndirectBayesTab(parent)
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
			m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
			m_dblManager->setDecimals(m_properties["ResBinning"], INT_DECIMALS);

			m_propTree->addProperty(m_properties["EMin"]);
			m_propTree->addProperty(m_properties["EMax"]);
			m_propTree->addProperty(m_properties["SampleBinning"]);
			m_propTree->addProperty(m_properties["ResBinning"]);

			//Set default values
			m_dblManager->setValue(m_properties["SampleBinning"], 1);
			m_dblManager->setValue(m_properties["ResBinning"], 1);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			m_plot->setCanvasBackground(Qt::white);
   		m_plot->setAxisFont(QwtPlot::xBottom, parent->font());
    	m_plot->setAxisFont(QwtPlot::yLeft, parent->font());
		}

		bool Quasi::validate()
		{
			return true;
		}

		void Quasi::run() 
		{

		}

		void Quasi::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["EMin"], min);
    }

    void Quasi::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["EMax"], max);	
    }

    void Quasi::updateProperties(QtProperty* prop, double val)
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
