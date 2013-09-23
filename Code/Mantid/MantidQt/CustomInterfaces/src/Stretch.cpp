#include "MantidQtCustomInterfaces/Stretch.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		Stretch::Stretch(QWidget * parent) : 
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
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
			m_dblManager->setMinimum(m_properties["Sigma"], 1);
			m_dblManager->setMaximum(m_properties["Sigma"], 200);
			m_dblManager->setValue(m_properties["Beta"], 50);
			m_dblManager->setMinimum(m_properties["Beta"], 1);
			m_dblManager->setMaximum(m_properties["Beta"], 200);
			m_dblManager->setValue(m_properties["SampleBinning"], 1);
			m_dblManager->setMinimum(m_properties["SampleBinning"], 1);

			//Connect the data selector for the sample to the mini plot
			connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(handleSampleInputReady(const QString&)));
		}

		/**
		 * Validate the form to check the program can be run
		 * 
		 * @return :: Whether the form was valid
		 */
		bool Stretch::validate()
		{
						//check that the sample file exists
			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString samplePath = m_uiForm.dsSample->getFullFilePath();

			if(!checkFileLoaded(sampleName, samplePath)) return false;

			//check that the resolution file exists
			QString resName = m_uiForm.dsResolution->getCurrentDataName();
			QString resPath = m_uiForm.dsResolution->getFullFilePath();

			if(!checkFileLoaded(resName, resPath)) return false;

			return true;
		}

		/**
		 * Collect the settings on the GUI and build a python
		 * script that runs Stretch
		 */
		void Stretch::run() 
		{
			QString save("False");
			QString verbose("False");

			QString elasticPeak("0");
			QString background("0");
			QString sequence("False");

			QString pyInput = 
				"from IndirectBayes import QuestRun\n";

			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString resName = m_uiForm.dsResolution->getCurrentDataName();

			// Collect input from options section
			QString backgroundTxt = m_uiForm.cbBackground->currentText();
			if(backgroundTxt == "Sloping")
			{
				background = "2";
			}
			else if( backgroundTxt == "Flat")
			{
				background = "1";
			}

			if(m_uiForm.chkElasticPeak->isChecked()) { elasticPeak = "1"; }
			if(m_uiForm.chkSequentialFit->isChecked()) { sequence = "True"; }

			QString fitOps = "[" + elasticPeak + ", " + background + ", 0, 0]";

			//Collect input from the properties browser
			QString eMin = m_properties["EMin"]->valueText();
			QString eMax = m_properties["EMax"]->valueText();
			QString eRange = "[" + eMin + "," + eMax + "]";

			QString beta = m_properties["Beta"]->valueText();
			QString sigma = m_properties["Sigma"]->valueText();
			QString betaSig = "[" + beta + ", " + sigma + "]";

			QString nBins = m_properties["SampleBinning"]->valueText();
			nBins = "[" + nBins + ", 1]";

			//Output options
			if(m_uiForm.chkVerbose->isChecked()) { verbose = "True"; }
			if(m_uiForm.chkSave->isChecked()) { save = "True"; }
			QString plot = m_uiForm.cbPlot->currentText();

			pyInput += "QuestRun('"+sampleName+"','"+resName+"',"+betaSig+","+eRange+","+nBins+","+fitOps+","+sequence+","
										" Save="+save+", Plot='"+plot+"', Verbose="+verbose+")\n";

			runPythonScript(pyInput);
		}

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 * 
		 * @param filename :: The name of the workspace to plot
		 */
		void Stretch::handleSampleInputReady(const QString& filename)
		{
			plotMiniPlot(filename, 0);
			std::pair<double,double> range = getCurveRange();
			setMiniPlotGuides(m_properties["EMin"], m_properties["EMax"], range);
			setPlotRange(m_properties["EMin"], m_properties["EMax"], range);
		}

		/**
		 * Updates the property manager when the lower guide is moved on the mini plot
		 *
		 * @param min :: The new value of the lower guide
		 */
		void Stretch::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["EMin"], min);
    }

		/**
		 * Updates the property manager when the upper guide is moved on the mini plot
		 *
		 * @param max :: The new value of the upper guide
		 */
    void Stretch::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["EMax"], max);	
    }

		/**
		 * Handles when properties in the property manager are updated.
		 *
		 * @param prop :: The property being updated
		 * @param val :: The new value for the property
		 */
    void Stretch::updateProperties(QtProperty* prop, double val)
    {
    	if(prop == m_properties["EMin"])
    	{
    		updateLowerGuide(m_properties["EMin"], m_properties["EMax"], val);
    	}
    	else if (prop == m_properties["EMax"])
    	{
				updateUpperGuide(m_properties["EMin"], m_properties["EMax"], val);
    	}
    }

	} // namespace CustomInterfaces
} // namespace MantidQt
