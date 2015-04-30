#include "MantidQtCustomInterfaces/Indirect/Stretch.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

namespace
{
  Mantid::Kernel::Logger g_log("Stretch");
}

namespace MantidQt
{
	namespace CustomInterfaces
	{
		Stretch::Stretch(QWidget * parent) :
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);


      // Create range selector
      auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("StretchERange");
      connect(eRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minValueChanged(double)));
      connect(eRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxValueChanged(double)));

			// Add the properties browser to the ui form
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
			connect(m_uiForm.chkSequentialFit, SIGNAL(toggled(bool)), m_uiForm.cbPlot, SLOT(setEnabled(bool)));
		}

    void Stretch::setup()
    {
    }

		/**
		 * Validate the form to check the program can be run
		 *
		 * @return :: Whether the form was valid
		 */
		bool Stretch::validate()
		{
			UserInputValidator uiv;
			uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
			uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

			QString errors = uiv.generateErrorMessage();
			if (!errors.isEmpty())
			{
				emit showMessageBox(errors);
				return false;
			}

			return true;
		}

		/**
		 * Collect the settings on the GUI and build a python
		 * script that runs Stretch
		 */
		void Stretch::run()
		{
      using namespace Mantid::API;

			QString save("False");

			QString elasticPeak("False");
			QString sequence("False");

			QString pyInput =
				"from IndirectBayes import QuestRun\n";

			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString resName = m_uiForm.dsResolution->getCurrentDataName();

			// Collect input from options section
			QString background = m_uiForm.cbBackground->currentText();

			if(m_uiForm.chkElasticPeak->isChecked()) { elasticPeak = "True"; }
			if(m_uiForm.chkSequentialFit->isChecked()) { sequence = "True"; }

			QString fitOps = "[" + elasticPeak + ", '" + background + "', False, False]";

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
			if(m_uiForm.chkSave->isChecked()) { save = "True"; }
			QString plot = m_uiForm.cbPlot->currentText();

			pyInput += "QuestRun('"+sampleName+"','"+resName+"',"+betaSig+","+eRange+","+nBins+","+fitOps+","+sequence+","
										" Save="+save+", Plot='"+plot+"')\n";

			runPythonScript(pyInput);
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *
     * @param settings :: The current settings
		 */
		void Stretch::loadSettings(const QSettings& settings)
		{
			m_uiForm.dsSample->readSettings(settings.group());
			m_uiForm.dsResolution->readSettings(settings.group());
		}

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 *
		 * @param filename :: The name of the workspace to plot
		 */
		void Stretch::handleSampleInputReady(const QString& filename)
		{
			m_uiForm.ppPlot->addSpectrum("Sample", filename, 0);
			QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
      auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");
			setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
			setPlotPropertyRange(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
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
      auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");

    	if(prop == m_properties["EMin"])
    	{
    		updateLowerGuide(eRangeSelector, m_properties["EMin"], m_properties["EMax"], val);
    	}
    	else if (prop == m_properties["EMax"])
    	{
				updateUpperGuide(eRangeSelector, m_properties["EMin"], m_properties["EMax"], val);
    	}
    }

	} // namespace CustomInterfaces
} // namespace MantidQt
