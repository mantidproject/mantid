#include "MantidQtCustomInterfaces/JumpFit.h"

#include <string>
#include <boost/lexical_cast.hpp>

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
			//check that the sample file is loaded
			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString samplePath = m_uiForm.dsSample->getFullFilePath();

			if(!checkFileLoaded(sampleName, samplePath)) return false;

			return true;
		}

		void JumpFit::run() 
		{
			QString verbose("False");
			QString plot("False");
			QString save("False");
			
			QString sample = m_uiForm.dsSample->getCurrentDataName();

			//fit function to use
			QString fitFunction("CE");
			switch(m_uiForm.cbFunction->currentIndex())
			{
				case 0:
					fitFunction = "CE"; // Use Chudley-Elliott
					break;
				case 1:
					fitFunction = "SS"; // Use Singwi-Sjolander
					break;
			}

			// width should be 0, 2 or 4
			int width = m_uiForm.cbWidth->currentIndex() * 2;
			QString widthTxt = boost::lexical_cast<std::string>(width).c_str();

			// Cropping values
			QString QMin = m_properties["QMin"]->valueText();
			QString QMax = m_properties["QMax"]->valueText();

			//output options
			if(m_uiForm.chkVerbose->isChecked()) { verbose = "True"; }
			if(m_uiForm.chkSave->isChecked()) { save = "True"; }
			if(m_uiForm.chkPlot->isChecked()) { plot = "True"; }

			QString pyInput = 
				"from IndirectJumpFit import JumpRun\n";

			pyInput += "JumpRun("+sample+","+fitFunction+","+widthTxt+","+QMin+","+QMax+","
									"Save="+save+", Plot='"+plot+"', Verbose="+verbose+")\n";

			runPythonScript(pyInput);
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
