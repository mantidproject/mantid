#include "MantidQtCustomInterfaces/DensityOfStates.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;

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

      connect(m_uiForm.pbAddIons, SIGNAL(clicked()), this, SLOT(addSelectedIons()));
      connect(m_uiForm.pbAddAllIons, SIGNAL(clicked()), this, SLOT(addAllIons()));
      connect(m_uiForm.pbRemoveIons, SIGNAL(clicked()), this, SLOT(removeSelectedIons()));
      connect(m_uiForm.pbRemoveAllIons, SIGNAL(clicked()), this, SLOT(removeAllIons()));

      m_uiForm.lwAllIons->setSelectionMode(QAbstractItemView::MultiSelection);
      m_uiForm.lwSelectedIons->setSelectionMode(QAbstractItemView::MultiSelection);
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
      //TODO
      return true;
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
      m_outputWsName = inputFileInfo.baseName();
      QString specType = m_uiForm.cbSpectrumType->currentText();

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
        //TODO

        dosAlgo->setProperty("SpectrumType", "DOS");
      }
      else if(specType == "IR")
      {
        dosAlgo->setProperty("SpectrumType", "IR_Active");
      }
      else if(specType == "Raman")
      {
        double temperature = m_uiForm.spTemperature->value();
        dosAlgo->setProperty("Temperature", temperature);

        dosAlgo->setProperty("SpectrumType", "Raman_Active");
      }

      m_batchAlgoRunner->addAlgorithm(dosAlgo);

      disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(ionLoadComplete(bool)));
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
      if(error)
        return;

      //TODO: plotting/saving
    }


    /**
     * Handles a new file being selected by the browser.
     */
    void DensityOfStates::handleFileChange()
    {
      QString filename = m_uiForm.mwInputFile->getFirstFilename();

      IAlgorithm_sptr ionTableAlgo = AlgorithmManager::Instance().create("DensityOfStates");
      ionTableAlgo->initialize();
      ionTableAlgo->setProperty("File", filename.toStdString());
      ionTableAlgo->setProperty("SpectrumType", "IonTable");
      ionTableAlgo->setProperty("OutputWorkspace", "__dos_ions");

      m_batchAlgoRunner->addAlgorithm(ionTableAlgo);

      disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(dosAlgoComplete(bool)));
      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(ionLoadComplete(bool)));
      m_batchAlgoRunner->executeBatchAsync();
    }


    /**
     * Handles the algorithm loading the list of ions in a file
     * being completed.
     *
     * @param error If the algorithm failed
     */
    void DensityOfStates::ionLoadComplete(bool error)
    {
      if(error)
        g_log.error("Could not get a list of ions from .phonon file");

      ITableWorkspace_sptr ionTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__dos_ions");
      Column_sptr ionColumn = ionTable->getColumn("Ion");
      size_t numIons = ionColumn->size();

      m_uiForm.lwAllIons->clear();
      m_uiForm.lwSelectedIons->clear();

      for(size_t ion = 0; ion < numIons; ion++)
      {
        const std::string ionName = ionColumn->cell<std::string>(ion);
        m_uiForm.lwAllIons->addItem(QString::fromStdString(ionName));
      }
    }


    /**
     * Handle adding all ions from the list of available ions
     * to the list of selected ions.
     */
    void DensityOfStates::addAllIons()
    {
      m_uiForm.lwAllIons->selectAll();
      addSelectedIons();
    }


    /**
     * Handle removing all ions from the list of selected ions.
     */
    void DensityOfStates::removeAllIons()
    {
      m_uiForm.lwSelectedIons->selectAll();
      removeSelectedIons();
    }


    /**
     * Handle adding selected ions from the list of available ions
     * to the list of selected ions.
     */
    void DensityOfStates::addSelectedIons()
    {
      auto items = m_uiForm.lwAllIons->selectedItems();

      for(auto it = items.begin(); it != items.end(); ++it)
      {
        m_uiForm.lwSelectedIons->addItem((*it)->text());
        m_uiForm.lwAllIons->takeItem(m_uiForm.lwAllIons->row(*it));
      }
    }


    /**
     * Handle removing selected ions from the list of selected ions.
     */
    void DensityOfStates::removeSelectedIons()
    {
      auto items = m_uiForm.lwSelectedIons->selectedItems();

      for(auto it = items.begin(); it != items.end(); ++it)
      {
        m_uiForm.lwAllIons->addItem((*it)->text());
        m_uiForm.lwSelectedIons->takeItem(m_uiForm.lwSelectedIons->row(*it));
      }
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
