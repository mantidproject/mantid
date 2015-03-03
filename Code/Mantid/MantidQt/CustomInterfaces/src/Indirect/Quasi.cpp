#include "MantidAPI/TextAxis.h"
#include "MantidQtCustomInterfaces/Indirect/Quasi.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

using namespace Mantid::API;

namespace MantidQt
{
	namespace CustomInterfaces
	{
		Quasi::Quasi(QWidget * parent) :
			IndirectBayesTab(parent),
      m_previewSpec(0)
		{
			m_uiForm.setupUi(parent);

      // Create range selector
      m_rangeSelectors["QuasiERange"] = new MantidWidgets::RangeSelector(m_uiForm.ppPlot);
      connect(m_rangeSelectors["QuasiERange"], SIGNAL(minValueChanged(double)), this, SLOT(minValueChanged(double)));
      connect(m_rangeSelectors["QuasiERange"], SIGNAL(maxValueChanged(double)), this, SLOT(maxValueChanged(double)));

			// Add the properties browser to the UI form
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

      // Connect the progrm selector to its handler
			connect(m_uiForm.cbProgram, SIGNAL(currentIndexChanged(int)), this, SLOT(handleProgramChange(int)));

      // Connect preview spectrum spinner to handler
      connect(m_uiForm.spPreviewSpectrum, SIGNAL(valueChanged(int)), this, SLOT(previewSpecChanged(int)));
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *
     * @param settings :: The current settings
		 */
		void Quasi::loadSettings(const QSettings& settings)
		{
			m_uiForm.dsSample->readSettings(settings.group());
			m_uiForm.dsResolution->readSettings(settings.group());
			m_uiForm.dsResNorm->readSettings(settings.group());
			m_uiForm.mwFixWidthDat->readSettings(settings.group());
		}

    void Quasi::setup()
    {
    }

		/**
		 * Validate the form to check the program can be run
		 *
		 * @return :: Whether the form was valid
		 */
		bool Quasi::validate()
		{
			UserInputValidator uiv;
			uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
			uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

			//check that the ResNorm file is valid if we are using it
			if(m_uiForm.chkUseResNorm->isChecked())
			{
				uiv.checkDataSelectorIsValid("ResNorm", m_uiForm.dsResNorm);
			}

			//check fixed width file exists
			if(m_uiForm.chkFixWidth->isChecked() &&
					 !m_uiForm.mwFixWidthDat->isValid())
			{
				uiv.checkMWRunFilesIsValid("Width", m_uiForm.mwFixWidthDat);
			}

			QString errors = uiv.generateErrorMessage();
			if (!errors.isEmpty())
			{
				emit showMessageBox(errors);
				return false;
			}

			QString program = m_uiForm.cbProgram->currentText();
      if(program == "Stretched Exponential")
      {
			  QString resName = m_uiForm.dsResolution->getCurrentDataName();
        if(!resName.endsWith("_res"))
        {
          emit showMessageBox("Stretched Exponential program can only be used with a resolution file.");
          return false;
        }
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

			if(program == "Lorentzians")
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

			// Collect input from the properties browser
			QString eMin = m_properties["EMin"]->valueText();
			QString eMax = m_properties["EMax"]->valueText();
			QString eRange = "[" + eMin + "," + eMax + "]";

			QString sampleBins = m_properties["SampleBinning"]->valueText();
			QString resBins = m_properties["ResBinning"]->valueText();
			QString nBins = "[" + sampleBins + "," + resBins + "]";

			// Output options
			if(m_uiForm.chkSave->isChecked()) { save = "True"; }
			QString plot = m_uiForm.cbPlot->currentText();

			pyInput += "QLRun('"+program+"','"+sampleName+"','"+resName+"','"+resNormFile+"',"+eRange+","
										" "+nBins+","+fitOps+",'"+fixedWidthFile+"',"+sequence+", "
										" Save="+save+", Plot='"+plot+"')\n";

			runPythonScript(pyInput);

      updateMiniPlot();
		}

    /**
     * Updates the data and fit curves on the mini plot.
     */
    void Quasi::updateMiniPlot()
    {
      // Update sample plot
      if(!m_uiForm.dsSample->isValid())
        return;

	  m_uiForm.ppPlot->clear();

      QString sampleName = m_uiForm.dsSample->getCurrentDataName();
	  m_uiForm.ppPlot->addSpectrum("Sample", sampleName, m_previewSpec);

      // Update fit plot
			QString program = m_uiForm.cbProgram->currentText();
			if(program == "Lorentzians")
				program = "QL";
			else
				program = "QSe";

			QString resName = m_uiForm.dsResolution->getCurrentDataName();

      // Should be either "red", "sqw" or "res"
      QString resType = resName.right(3);

      // Get the correct workspace name based on the type of resolution file
      if(program == "QL")
      {
        if(resType == "res")
          program += "r";
        else
          program += "d";
      }

      QString outWsName = sampleName.left(sampleName.size() - 3) + program + "_Workspace_" + QString::number(m_previewSpec);
      if(!AnalysisDataService::Instance().doesExist(outWsName.toStdString()))
        return;

      MatrixWorkspace_sptr outputWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName.toStdString());

      TextAxis* axis = dynamic_cast<TextAxis*>(outputWorkspace->getAxis(1));

      for(size_t histIndex = 0; histIndex < outputWorkspace->getNumberHistograms(); histIndex++)
      {
        QString specName = QString::fromStdString(axis->label(histIndex));
        QColor curveColour;

        if(specName.contains("fit.1"))
          curveColour = Qt::red;
        else if(specName.contains("fit.1"))
          curveColour = Qt::magenta;

        else if(specName.contains("diff.1"))
          curveColour = Qt::green;
        else if(specName.contains("diff.1"))
          curveColour = Qt::cyan;

        else
          continue;

        m_uiForm.ppPlot->addSpectrum(specName, outputWorkspace, histIndex, curveColour);
      }
    }

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 *
		 * @param filename :: The name of the workspace to plot
		 */
		void Quasi::handleSampleInputReady(const QString& filename)
		{
      MatrixWorkspace_sptr inWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(filename.toStdString());
      int numHist = static_cast<int>(inWs->getNumberHistograms()) - 1;
      m_uiForm.spPreviewSpectrum->setMaximum(numHist);
      updateMiniPlot();

			QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
			setRangeSelector("QuasiERange", m_properties["EMin"], m_properties["EMax"], range);
			setPlotPropertyRange("QuasiERange", m_properties["EMin"], m_properties["EMax"], range);
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
    		updateLowerGuide(m_rangeSelectors["QuasiERange"], m_properties["EMin"], m_properties["EMax"], val);
    	}
    	else if (prop == m_properties["EMax"])
    	{
				updateUpperGuide(m_rangeSelectors["QuasiERange"], m_properties["EMin"], m_properties["EMax"], val);
    	}
    }

		/**
		 * Handles when the slected item in the program combobox
		 * is changed
		 *
		 * @param index :: The current index of the combobox
		 */
    void Quasi::handleProgramChange(int index)
    {
    	int numberOptions = m_uiForm.cbPlot->count();
    	switch(index)
    	{
    		case 0:
    			m_uiForm.cbPlot->setItemText(numberOptions-1, "Prob");
    			break;
    		case 1:
    			m_uiForm.cbPlot->setItemText(numberOptions-1, "Beta");
    			break;
    	}
    }

    /**
     * Handles setting a new preview spectrum on the preview plot.
     *
     * @param value Spectrum index
     */
    void Quasi::previewSpecChanged(int value)
    {
      m_previewSpec = value;
      updateMiniPlot();
    }

	} // namespace CustomInterfaces
} // namespace MantidQt
