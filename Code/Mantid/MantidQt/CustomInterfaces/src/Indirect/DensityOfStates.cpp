#include "MantidQtCustomInterfaces/DensityOfStates.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace
{
  Mantid::Kernel::Logger g_log("DensityOfStates");
}


namespace MantidQt
{
	namespace CustomInterfaces
	{
		DensityOfStates::DensityOfStates(QWidget * parent) :
			IndirectSimulationTab(parent)
		{
			m_uiForm.setupUi(parent);

      connect(m_uiForm.mwInputFile, SIGNAL(filesFound()), this, SLOT(handleFileChange()));

      m_uiForm.lwIons->setSelectionMode(QAbstractItemView::MultiSelection);
		}


    void DensityOfStates::setup()
    {
    }


    /**
     * Validate the form to check the program can be run.
     *
     * @return Whether the form was valid
     */
		bool DensityOfStates::validate()
		{
      UserInputValidator uiv;

      // Ensure there are ions selected when using DensityOfStates sectrum with .phonon file
      QString filename = m_uiForm.mwInputFile->getFirstFilename();
      QFileInfo fileInfo(filename);
      bool canDoPartialDoS = fileInfo.suffix() == "phonon";

      QString specType = m_uiForm.cbSpectrumType->currentText();
      auto items = m_uiForm.lwIons->selectedItems();

      if(specType == "DensityOfStates" && canDoPartialDoS && items.size() < 1)
        uiv.addErrorMessage("Must select at least one ion for DensityOfStates.");

      // Give error message when there are errors
      if(!uiv.isAllInputValid())
        emit showMessageBox(uiv.generateErrorMessage());

      return uiv.isAllInputValid();
		}


    /**
     * Configures and executes the LoadSassena algorithm.
     */
		void DensityOfStates::run()
    {
      // Get the DensityOfStates algorithm
      IAlgorithm_sptr dosAlgo = AlgorithmManager::Instance().create("DensityOfStates");

      QString filename = m_uiForm.mwInputFile->getFirstFilename();
      QFileInfo inputFileInfo(filename);
      QString specType = m_uiForm.cbSpectrumType->currentText();

      m_outputWsName = inputFileInfo.baseName() + "_" + specType;

      // Set common properties
      dosAlgo->setProperty("File", filename.toStdString());
      dosAlgo->setProperty("OutputWorkspace", m_outputWsName.toStdString());

      QString peakShape = m_uiForm.cbPeakShape->currentText();
      dosAlgo->setProperty("Function", peakShape.toStdString());

      double peakWidth = m_uiForm.spPeakWidth->value();
      dosAlgo->setProperty("PeakWidth", peakWidth);

      double binWidth = m_uiForm.spBinWidth->value();
      dosAlgo->setProperty("BinWidth", binWidth);

      double zeroThreshold = m_uiForm.spZeroThreshold->value();
      dosAlgo->setProperty("ZeroThreshold", zeroThreshold);

      bool scale = m_uiForm.ckScale->isChecked();
      double scaleFactor = m_uiForm.spScale->value();
      if(scale)
        dosAlgo->setProperty("Scale", scaleFactor);

      // Set spectrum type specific properties
      if(specType == "DensityOfStates")
      {
        dosAlgo->setProperty("SpectrumType", "DOS");

        bool crossSectionScale = m_uiForm.ckCrossSectionScale->isChecked();
        QString crossSectionScaleType = m_uiForm.cbCrossSectionScale->currentText();
        if(crossSectionScale)
          dosAlgo->setProperty("ScaleByCrossSection", crossSectionScaleType.toStdString());

        bool sumContributions = m_uiForm.ckSumContributions->isChecked();
        dosAlgo->setProperty("SumContributions", sumContributions);

        std::vector<std::string> selectedIons;
        auto items = m_uiForm.lwIons->selectedItems();
        for(auto it = items.begin(); it != items.end(); ++it)
          selectedIons.push_back((*it)->text().toStdString());
        dosAlgo->setProperty("Ions", selectedIons);
      }
      else if(specType == "IR")
      {
        dosAlgo->setProperty("SpectrumType", "IR_Active");
      }
      else if(specType == "Raman")
      {
        dosAlgo->setProperty("SpectrumType", "Raman_Active");

        double temperature = m_uiForm.spTemperature->value();
        dosAlgo->setProperty("Temperature", temperature);
      }

      m_batchAlgoRunner->addAlgorithm(dosAlgo);

      // Setup save algorithm if needed
      if(m_uiForm.ckSave->isChecked())
      {
        BatchAlgorithmRunner::AlgorithmRuntimeProps saveProps;
        saveProps["InputWorkspace"] = m_outputWsName.toStdString();

        QString filename = m_outputWsName + ".nxs";

        IAlgorithm_sptr saveAlgo = AlgorithmManager::Instance().create("SaveNexusProcessed");
        saveAlgo->setProperty("Filename", filename.toStdString());

        m_batchAlgoRunner->addAlgorithm(saveAlgo, saveProps);
      }

      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(dosAlgoComplete(bool)));
      m_batchAlgoRunner->executeBatchAsync();
		}


    /**
     * Handles completion of the DensityOfStates algorithm.
     *
     * @param error If the algorithm failed
     */
    void DensityOfStates::dosAlgoComplete(bool error)
    {
      disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(dosAlgoComplete(bool)));

      if(error)
        return;

      // Handle spectra plotting
      if(m_uiForm.ckPlot->isChecked())
      {
        QString pyInput = "from mantidplot import plotSpectrum, plot2D\n"
                          "plotSpectrum('" + m_outputWsName + "', 0)\n";

        runPythonCode(pyInput);
      }
    }


    /**
     * Handles a new file being selected by the browser.
     */
    void DensityOfStates::handleFileChange()
    {
      QString filename = m_uiForm.mwInputFile->getFirstFilename();

      // Check if we have a .phonon file
      QFileInfo fileInfo(filename);
      bool canDoPartialDoS = fileInfo.suffix() == "phonon";

      // Need a .phonon file for ion contributions
      if(canDoPartialDoS)
      {
        // Load the ion table to populate the list of ions
        IAlgorithm_sptr ionTableAlgo = AlgorithmManager::Instance().create("DensityOfStates");
        ionTableAlgo->initialize();
        ionTableAlgo->setProperty("File", filename.toStdString());
        ionTableAlgo->setProperty("SpectrumType", "IonTable");
        ionTableAlgo->setProperty("OutputWorkspace", "__dos_ions");

        m_batchAlgoRunner->addAlgorithm(ionTableAlgo);

        connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(ionLoadComplete(bool)));
        m_batchAlgoRunner->executeBatchAsync();
      }
      else
      {
        m_uiForm.lwIons->clear();
        m_uiForm.ckSumContributions->setChecked(false);
        m_uiForm.ckCrossSectionScale->setChecked(false);
      }

      // Enable partial DOS related optons when they can be used
      m_uiForm.lwIons->setEnabled(canDoPartialDoS);
      m_uiForm.pbSelectAllIons->setEnabled(canDoPartialDoS);
      m_uiForm.pbDeselectAllIons->setEnabled(canDoPartialDoS);
      m_uiForm.ckSumContributions->setEnabled(canDoPartialDoS);
      m_uiForm.ckCrossSectionScale->setEnabled(canDoPartialDoS);
    }


    /**
     * Handles the algorithm loading the list of ions in a file
     * being completed.
     *
     * @param error If the algorithm failed
     */
    void DensityOfStates::ionLoadComplete(bool error)
    {
      disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(ionLoadComplete(bool)));

      if(error)
        g_log.error("Could not get a list of ions from .phonon file");

      // Get the list of ions from algorithm
      ITableWorkspace_sptr ionTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__dos_ions");
      Column_sptr ionColumn = ionTable->getColumn("Ion");
      size_t numIons = ionColumn->size();

      // Remove old ions
      m_uiForm.lwIons->clear();

      // Add ions to list
      for(size_t ion = 0; ion < numIons; ion++)
      {
        const std::string ionName = ionColumn->cell<std::string>(ion);
        m_uiForm.lwIons->addItem(QString::fromStdString(ionName));
      }

      // Select all ions by default
      m_uiForm.lwIons->selectAll();
    }


    /**
     * Set the data selectors to use the default save directory
     * when browsing for input files.
     *
     * @param settings :: The settings to loading into the interface
     */
		void DensityOfStates::loadSettings(const QSettings& settings)
		{
			m_uiForm.mwInputFile->readSettings(settings.group());
		}

	} // namespace CustomInterfaces
} // namespace MantidQt
