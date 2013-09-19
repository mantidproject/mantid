#include "MantidQtCustomInterfaces/JumpFit.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		JumpFit::JumpFit(QWidget * parent) : 
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			//add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);

			m_properties["QMin"] = m_dblManager->addProperty("QMin");
			m_properties["QMax"] = m_dblManager->addProperty("QMax");
			
			m_dblManager->setDecimals(m_properties["QMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["QMax"], NUM_DECIMALS);

			m_propTree->addProperty(m_properties["QMin"]);
			m_propTree->addProperty(m_properties["QMax"]);

			// Connect data selector to handler method
			connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(handleSampleInputReady(const QString&)));
		}

		bool JumpFit::validate()
		{
			return true;
		}

		void JumpFit::run() 
		{

		}

		void JumpFit::handleSampleInputReady(const QString& filename)
		{
			plotMiniPlot(filename, 0);
			std::pair<double,double> res;
			std::pair<double,double> range = getCurveRange();

			//Use the values from the instrument parameter file if we can
			if(getInstrumentResolution(filename, res))
			{
				setMiniPlotGuides(m_properties["QMin"], m_properties["QMax"], res);
			}
			else
			{
				setMiniPlotGuides(m_properties["QMin"], m_properties["QMax"], range);
			}

			setPlotRange(m_properties["QMin"], m_properties["QMax"], range);
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
    		updateLowerGuide(m_properties["QMin"], m_properties["QMax"], val);
    	}
    	else if (prop == m_properties["QMax"])
    	{
				updateUpperGuide(m_properties["QMin"], m_properties["QMax"], val);
    	}
    }
	} // namespace CustomInterfaces
} // namespace MantidQt
