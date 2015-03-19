#include "MantidQtCustomInterfaces/Indirect/ResNorm.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

using namespace Mantid::API;

namespace MantidQt
{
	namespace CustomInterfaces
	{
		ResNorm::ResNorm(QWidget * parent) :
			IndirectBayesTab(parent),
      m_previewSpec(0)
		{
			m_uiForm.setupUi(parent);

      // Create range selector
      auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("ResNormERange");
      connect(eRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minValueChanged(double)));
      connect(eRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxValueChanged(double)));

			// Add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);

			m_properties["EMin"] = m_dblManager->addProperty("EMin");
			m_properties["EMax"] = m_dblManager->addProperty("EMax");
			m_properties["VanBinning"] = m_dblManager->addProperty("Van Binning");

			m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["VanBinning"], INT_DECIMALS);

			m_propTree->addProperty(m_properties["EMin"]);
			m_propTree->addProperty(m_properties["EMax"]);
			m_propTree->addProperty(m_properties["VanBinning"]);

			//set default values
			m_dblManager->setValue(m_properties["VanBinning"], 1);
			m_dblManager->setMinimum(m_properties["VanBinning"], 1);

			// Connect data selector to handler method
			connect(m_uiForm.dsVanadium, SIGNAL(dataReady(const QString&)), this, SLOT(handleVanadiumInputReady(const QString&)));

      // Connect the preview spectrum selector
      connect(m_uiForm.spPreviewSpectrum, SIGNAL(valueChanged(int)), this, SLOT(previewSpecChanged(int)));
		}

    void ResNorm::setup()
    {
    }

		/**
		 * Validate the form to check the program can be run
		 *
		 * @return :: Whether the form was valid
		 */
		bool ResNorm::validate()
		{
			UserInputValidator uiv;
			uiv.checkDataSelectorIsValid("Vanadium", m_uiForm.dsVanadium);
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
		 * script that runs ResNorm
		 */
		void ResNorm::run()
		{
			QString save("False");

			QString pyInput =
				"from IndirectBayes import ResNormRun\n";

			// get the file names
			QString VanName = m_uiForm.dsVanadium->getCurrentDataName();
			QString ResName = m_uiForm.dsResolution->getCurrentDataName();

			// get the parameters for ResNorm
			QString EMin = m_properties["EMin"]->valueText();
			QString EMax = m_properties["EMax"]->valueText();

			QString ERange = "[" + EMin + "," + EMax + "]";

			QString nBin = m_properties["VanBinning"]->valueText();

			// get output options
			if(m_uiForm.chkSave->isChecked()){ save ="True"; }
			QString plot = m_uiForm.cbPlot->currentText();

			pyInput += "ResNormRun('"+VanName+"', '"+ResName+"', "+ERange+", "+nBin+","
										" Save="+save+", Plot='"+plot+"')\n";

			runPythonScript(pyInput);

      // Plot the fit curve
      m_uiForm.ppPlot->addSpectrum("Fit", "Fit", m_previewSpec, Qt::red);
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *
     * @param settings :: The current settings
		 */
		void ResNorm::loadSettings(const QSettings& settings)
		{
			m_uiForm.dsVanadium->readSettings(settings.group());
    	m_uiForm.dsResolution->readSettings(settings.group());
		}

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 *
		 * @param filename :: The name of the workspace to plot
		 */
		void ResNorm::handleVanadiumInputReady(const QString& filename)
		{
			m_uiForm.ppPlot->addSpectrum("Vanadium", filename, m_previewSpec);
			QPair<double, double> res;
			QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Vanadium");

      MatrixWorkspace_sptr vanWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(filename.toStdString());
      m_uiForm.spPreviewSpectrum->setMaximum(static_cast<int>(vanWs->getNumberHistograms()) - 1);

      auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormERange");

			//Use the values from the instrument parameter file if we can
			if(getInstrumentResolution(filename, res))
			{
				//ResNorm resolution should be +/- 10 * the IPF resolution
				res.first = res.first * 10;
				res.second = res.second * 10;

				setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"], res);
			}
			else
			{
				setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
			}

			setPlotPropertyRange(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
		}

		/**
		 * Updates the property manager when the lower guide is moved on the mini plot
		 *
		 * @param min :: The new value of the lower guide
		 */
		void ResNorm::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["EMin"], min);
    }

		/**
		 * Updates the property manager when the upper guide is moved on the mini plot
		 *
		 * @param max :: The new value of the upper guide
		 */
    void ResNorm::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["EMax"], max);
    }

		/**
		 * Handles when properties in the property manager are updated.
		 *
		 * @param prop :: The property being updated
		 * @param val :: The new value for the property
		 */
    void ResNorm::updateProperties(QtProperty* prop, double val)
    {
      auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormERange");

    	if(prop == m_properties["EMin"])
    	{
				updateLowerGuide(eRangeSelector, m_properties["EMin"], m_properties["EMax"], val);
    	}
    	else if (prop == m_properties["EMax"])
    	{
    		updateUpperGuide(eRangeSelector, m_properties["EMin"], m_properties["EMax"], val);
			}
    }

    /**
     * Sets a new preview spectrum for the mini plot.
     *
     * @param value Spectrum index
     */
    void ResNorm::previewSpecChanged(int value)
    {
      m_previewSpec = value;

      if(m_uiForm.dsVanadium->isValid())
  			m_uiForm.ppPlot->addSpectrum("Vanadium", m_uiForm.dsVanadium->getCurrentDataName(), m_previewSpec);

      if(AnalysisDataService::Instance().doesExist("Fit"))
        m_uiForm.ppPlot->addSpectrum("Fit", "Fit", m_previewSpec, Qt::red);
    }

	} // namespace CustomInterfaces
} // namespace MantidQt
