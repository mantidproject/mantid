#include "MantidQtCustomInterfaces/Quasi.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		Quasi::Quasi(QWidget * parent) : 
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
			m_dblManager->setMinimum(m_properties["SampleBinning"], 1);
			m_dblManager->setValue(m_properties["ResBinning"], 1);
			m_dblManager->setMinimum(m_properties["ResBinning"], 1);

			//Connect optional form elements with enabling checkboxes
			connect(m_uiForm.chkFixWidth, SIGNAL(toggled(bool)), m_uiForm.mwFixWidthDat, SLOT(setEnabled(bool)));
			connect(m_uiForm.chkUseResNorm, SIGNAL(toggled(bool)), m_uiForm.dsResNorm, SLOT(setEnabled(bool)));

			//Connect the data selector for the sample to the mini plot
			connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(handleSampleInputReady(const QString&)));
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *  
		 * @param filename :: The name of the workspace to plot
		 */
		void Quasi::loadSettings(const QSettings& settings)
		{
			m_uiForm.dsSample->readSettings(settings.group());
			m_uiForm.dsResolution->readSettings(settings.group());
			m_uiForm.dsResNorm->readSettings(settings.group());
			m_uiForm.mwFixWidthDat->readSettings(settings.group());
		}

		/**
		 * Validate the form to check the program can be run
		 * 
		 * @return :: Whether the form was valid
		 */
		bool Quasi::validate()
		{
			//check that the sample file exists
			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString samplePath = m_uiForm.dsSample->getFullFilePath();

			if(!checkFileLoaded(sampleName, samplePath)) return false;

			//check that the resolution file exists
			QString resName = m_uiForm.dsResolution->getCurrentDataName();
			QString resPath = m_uiForm.dsResolution->getFullFilePath();

			if(!checkFileLoaded(resName, resPath)) return false;

			//check that the ResNorm file is valid if we are using it
			if(m_uiForm.chkUseResNorm->isChecked())
			{
				QString resNormFile = m_uiForm.dsResNorm->getCurrentDataName();
				QString resNormPath = m_uiForm.dsResNorm->getFullFilePath();

				if(!checkFileLoaded(resNormFile, resNormPath)) return false;
			}

			//check fixed width file exists
			if(m_uiForm.chkFixWidth->isChecked() &&
					 !m_uiForm.mwFixWidthDat->isValid())
			{
				emit showMessageBox("Please correct the following:\n Could not find the specified Fixed Width file");
				return false;
			}

			return true;
		}

		/**
		 * Collect the settings on the GUI and build a python
		 * script that runs Quasi
		 */
		void Quasi::run() 
		{
			// Using 1/0 instead of True/False for compatibility with underlying Fortran code
			// in some places
			QString verbose("False");
			QString save("False");
			QString elasticPeak("False");
			QString sequence("False");

			QString fixedWidth("False");
			QString fixedWidthFile("");

			QString useResNorm("False");
			QString resNormFile("");

			QString pyInput = 
				"from IndirectBayes import QLRun\n";

			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString resName = m_uiForm.dsResolution->getCurrentDataName();

			QString program = m_uiForm.cbProgram->currentText();

			if(program == "Lorenzians")
			{
				program = "QL";
			}
			else
			{
				program = "QSe";
			}

			// Collect input from fit options section
			QString background = m_uiForm.cbBackground->currentText();

			if(m_uiForm.chkElasticPeak->isChecked()) { elasticPeak = "True"; }
			if(m_uiForm.chkSequentialFit->isChecked()) { sequence = "True"; }

			if(m_uiForm.chkFixWidth->isChecked()) 
			{ 
				fixedWidth = "True";
				fixedWidthFile = m_uiForm.mwFixWidthDat->getFirstFilename();
			}

			if(m_uiForm.chkUseResNorm->isChecked())
			{
				useResNorm = "True";
				resNormFile = m_uiForm.dsResNorm->getCurrentDataName();
			}

			QString fitOps = "[" + elasticPeak + ", '" + background + "', " + fixedWidth + ", " + useResNorm + "]";

			//Collect input from the properties browser
			QString eMin = m_properties["EMin"]->valueText();
			QString eMax = m_properties["EMax"]->valueText();
			QString eRange = "[" + eMin + "," + eMax + "]";

			QString sampleBins = m_properties["SampleBinning"]->valueText();
			QString resBins = m_properties["ResBinning"]->valueText();
			QString nBins = "[" + sampleBins + "," + resBins + "]";

			//Output options
			if(m_uiForm.chkVerbose->isChecked()) { verbose = "True"; }
			if(m_uiForm.chkSave->isChecked()) { save = "True"; }
			QString plot = m_uiForm.cbPlot->currentText();

			pyInput += "QLRun('"+program+"','"+sampleName+"','"+resName+"','"+resNormFile+"',"+eRange+","
										" "+nBins+","+fitOps+",'"+fixedWidthFile+"',"+sequence+", "
										" Save="+save+", Plot='"+plot+"', Verbose="+verbose+")\n";

			runPythonScript(pyInput);
		}

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 * 
		 * @param filename :: The name of the workspace to plot
		 */
		void Quasi::handleSampleInputReady(const QString& filename)
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
		void Quasi::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["EMin"], min);
    }

		/**
		 * Updates the property manager when the upper guide is moved on the mini plot
		 *
		 * @param max :: The new value of the upper guide
		 */
    void Quasi::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["EMax"], max);	
    }

		/**
		 * Handles when properties in the property manager are updated.
		 *
		 * @param prop :: The property being updated
		 * @param val :: The new value for the property
		 */
    void Quasi::updateProperties(QtProperty* prop, double val)
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
