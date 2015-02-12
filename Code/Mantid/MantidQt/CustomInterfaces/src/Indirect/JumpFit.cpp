#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidQtCustomInterfaces/Indirect/JumpFit.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <string>
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;

namespace MantidQt
{
	namespace CustomInterfaces
	{
		JumpFit::JumpFit(QWidget * parent) :
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

      // Create range selector
      m_rangeSelectors["JumpFitQ"] = new MantidWidgets::RangeSelector(m_uiForm.ppPlot);
      connect(m_rangeSelectors["JumpFitQ"], SIGNAL(selectionChangedLazy(double, double)), this, SLOT(qRangeChanged(double, double)));

			// Add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);

			m_properties["QMin"] = m_dblManager->addProperty("QMin");
			m_properties["QMax"] = m_dblManager->addProperty("QMax");

			m_dblManager->setDecimals(m_properties["QMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["QMax"], NUM_DECIMALS);

			m_propTree->addProperty(m_properties["QMin"]);
			m_propTree->addProperty(m_properties["QMax"]);

			m_uiForm.cbWidth->setEnabled(false);

			// Connect data selector to handler method
			connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(handleSampleInputReady(const QString&)));
			// Connect width selector to handler method
			connect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(handleWidthChange(const QString&)));

      // Connect algorithm runner to completion handler function
      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(fitAlgDone(bool)));
		}

    void JumpFit::setup()
    {
    }

		/**
		 * Validate the form to check the program can be run
		 *
		 * @return :: Whether the form was valid
		 */
		bool JumpFit::validate()
		{
			UserInputValidator uiv;
			uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

			//this workspace doesn't have any valid widths
			if(m_spectraList.size() == 0)
			{
				uiv.addErrorMessage("Input workspace doesn't appear to contain any width data.");
			}

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
		 * script that runs JumpFit
		 */
		void JumpFit::run()
		{
			bool verbose = m_uiForm.chkVerbose->isChecked();
			bool save = m_uiForm.chkSave->isChecked();
			bool plot = m_uiForm.chkPlot->isChecked();

      runImpl(verbose, plot, save);
		}

    /**
     * Runs the JumpFit algorithm with preview parameters to update the preview plot.
     */
    void JumpFit::runPreviewAlgorithm()
    {
      runImpl();
    }

    /**
     * Runs algorithm.
     *
     * @param verbose Enable/disable verbose option
     * @param plot Enable/disable plotting
     * @param save Enable/disable saving
     */
    void JumpFit::runImpl(bool verbose, bool plot, bool save)
    {
      // Do noting with invalid data
			if(!m_uiForm.dsSample->isValid())
        return;

			// Fit function to use
			QString fitFunction("ChudleyElliot");
			switch(m_uiForm.cbFunction->currentIndex())
			{
				case 0:
					fitFunction = "ChudleyElliot";
					break;
				case 1:
					fitFunction = "HallRoss";
					break;
				case 2:
					fitFunction = "FickDiffusion";
					break;
				case 3:
					fitFunction = "TeixeiraWater";
					break;
			}

      // Loaded workspace name
			QString sample = m_uiForm.dsSample->getCurrentDataName();
			auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(sample.toStdString());

			std::string widthText = m_uiForm.cbWidth->currentText().toStdString();
      long width = m_spectraList[widthText];

      fitAlg = AlgorithmManager::Instance().create("JumpFit");
      fitAlg->initialize();

      fitAlg->setProperty("InputWorkspace", ws);
      fitAlg->setProperty("Function", fitFunction.toStdString());

      fitAlg->setProperty("Width", width);
      fitAlg->setProperty("QMin", m_dblManager->value(m_properties["QMin"]));
      fitAlg->setProperty("QMax", m_dblManager->value(m_properties["QMax"]));

      fitAlg->setProperty("Verbose", verbose);
      fitAlg->setProperty("Plot", plot);
      fitAlg->setProperty("Save", save);

      if(m_batchAlgoRunner->queueLength() < 1)
        runAlgorithm(fitAlg);
    }

    /**
     * Handles the JumpFit algorithm finishing, used to plot fit in miniplot.
     *
     * @param error True if the algorithm failed, false otherwise
     */
    void JumpFit::fitAlgDone(bool error)
    {
      // Ignore errors
      if(error)
        return;

      std::string outWsName = fitAlg->getPropertyValue("Output") + "_Workspace";
      MatrixWorkspace_sptr outputWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
      TextAxis* axis = dynamic_cast<TextAxis*>(outputWorkspace->getAxis(1));

      for(unsigned int histIndex = 0; histIndex < outputWorkspace->getNumberHistograms(); histIndex++)
      {
        QString specName = QString::fromStdString(axis->label(histIndex));

        if(specName == "Calc")
          m_uiForm.ppPlot->addSpectrum("Fit", outputWorkspace, histIndex, Qt::red);

        if(specName == "Diff")
          m_uiForm.ppPlot->addSpectrum("Diff", outputWorkspace, histIndex, Qt::green);
      }
    }

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *
     * @param settings :: The current settings
		 */
		void JumpFit::loadSettings(const QSettings& settings)
		{
			m_uiForm.dsSample->readSettings(settings.group());
		}

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 *
		 * @param filename :: The name of the workspace to plot
		 */
		void JumpFit::handleSampleInputReady(const QString& filename)
		{
      // Disable things that run the preview algorithm
      disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(runPreviewAlgorithm()));
			disconnect(m_uiForm.cbFunction, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(runPreviewAlgorithm()));
			disconnect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(runPreviewAlgorithm()));

      // Scale to convert to HWHM
      IAlgorithm_sptr scaleAlg = AlgorithmManager::Instance().create("Scale");
      scaleAlg->initialize();
      scaleAlg->setProperty("InputWorkspace", filename.toStdString());
      scaleAlg->setProperty("OutputWorkspace", filename.toStdString());
      scaleAlg->setProperty("Factor", 0.5);
      scaleAlg->execute();

			auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(filename.toStdString());
			auto mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);

			findAllWidths(mws);

			if(m_spectraList.size() > 0)
			{
				m_uiForm.cbWidth->setEnabled(true);

				std::string currentWidth = m_uiForm.cbWidth->currentText().toStdString();

        m_uiForm.ppPlot->clear();
        m_uiForm.ppPlot->addSpectrum("Sample", filename, m_spectraList[currentWidth]);

				std::pair<double, double> res;
				QPair<double, double> curveRange = m_uiForm.ppPlot->getCurveRange("Sample");
        std::pair<double, double> range(curveRange.first, curveRange.second);

				// Use the values from the instrument parameter file if we can
				if(getInstrumentResolution(filename, res))
					setMiniPlotGuides("JumpFitQ", m_properties["QMin"], m_properties["QMax"], res);
				else
					setMiniPlotGuides("JumpFitQ", m_properties["QMin"], m_properties["QMax"], range);

				setPlotRange("JumpFitQ", m_properties["QMin"], m_properties["QMax"], range);
			}
			else
			{
				m_uiForm.cbWidth->setEnabled(false);
				emit showMessageBox("Workspace doesn't appear to contain any width data");
			}

      // Update preview plot
      runPreviewAlgorithm();

      // Re-enable things that run the preview algorithm
      connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(runPreviewAlgorithm()));
			connect(m_uiForm.cbFunction, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(runPreviewAlgorithm()));
			connect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(runPreviewAlgorithm()));
		}

		/**
		 * Find all of the spectra in the workspace that have width data
		 *
		 * @param ws :: The workspace to search
		 */
		void JumpFit::findAllWidths(Mantid::API::MatrixWorkspace_const_sptr ws)
		{
			m_uiForm.cbWidth->clear();
			m_spectraList.clear();

			for (size_t i = 0; i < ws->getNumberHistograms(); ++i)
			{
				auto axis = dynamic_cast<Mantid::API::TextAxis*>(ws->getAxis(1));
        if(!axis)
          return;

        std::string title = axis->label(i);

				//check if the axis labels indicate this spectrum is width data
				size_t qLinesWidthIndex = title.find(".Width");
				size_t convFitWidthIndex = title.find(".FWHM");

				bool qLinesWidth = qLinesWidthIndex != std::string::npos;
				bool convFitWidth = convFitWidthIndex != std::string::npos;

				//if we get a match, add this spectrum to the combobox
				if(convFitWidth || qLinesWidth)
				{
					std::string cbItemName = "";
					size_t substrIndex = 0;

					if (qLinesWidth)
					{
						substrIndex = qLinesWidthIndex;
					}
					else if (convFitWidth)
					{
						substrIndex = convFitWidthIndex;
					}

					cbItemName = title.substr(0, substrIndex);
					m_spectraList[cbItemName] = static_cast<int>(i);
					m_uiForm.cbWidth->addItem(QString(cbItemName.c_str()));

					//display widths f1.f1, f2.f1 and f2.f2
					if (m_uiForm.cbWidth->count() == 3)
					{
						return;
					}
				}
			}
		}

		/**
		 * Plots the loaded file to the miniplot when the selected spectrum changes
		 *
		 * @param text :: The name spectrum index to plot
		 */
		void JumpFit::handleWidthChange(const QString& text)
		{
			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString samplePath = m_uiForm.dsSample->getFullFilePath();

			if(!sampleName.isEmpty() && m_spectraList.size() > 0)
			{
				if(validate())
				{
          m_uiForm.ppPlot->clear();
          m_uiForm.ppPlot->addSpectrum("Sample", sampleName, m_spectraList[text.toStdString()]);
				}
			}
		}

		/**
		 * Updates the property manager when the range selector is moved on the mini plot.
		 *
		 * @param min :: The new value of the lower guide
		 * @param max :: The new value of the upper guide
		 */
		void JumpFit::qRangeChanged(double min, double max)
    {
      m_dblManager->setValue(m_properties["QMin"], min);
			m_dblManager->setValue(m_properties["QMax"], max);
    }

		/**
		 * Handles when properties in the property manager are updated.
		 *
		 * @param prop :: The property being updated
		 * @param val :: The new value for the property
		 */
    void JumpFit::updateProperties(QtProperty* prop, double val)
    {
    	if(prop == m_properties["QMin"])
    	{
    		updateLowerGuide(m_rangeSelectors["JumpFitQ"], m_properties["QMin"], m_properties["QMax"], val);
    	}
    	else if (prop == m_properties["QMax"])
    	{
				updateUpperGuide(m_rangeSelectors["JumpFitQ"], m_properties["QMin"], m_properties["QMax"], val);
    	}
    }
	} // namespace CustomInterfaces
} // namespace MantidQt
