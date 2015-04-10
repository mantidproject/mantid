#include "MantidQtCustomInterfaces/Indirect/IndirectSassena.h"

#include <QFileInfo>
#include <QString>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		IndirectSassena::IndirectSassena(QWidget * parent) :
			IndirectSimulationTab(parent)
		{
			m_uiForm.setupUi(parent);
      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(handleAlgorithmFinish(bool)));
		}

    void IndirectSassena::setup()
    {
    }

    /**
     * Validate the form to check the program can be run.
     *
     * @return Whether the form was valid
     */
		bool IndirectSassena::validate()
		{
      // There is very little to actually be invalid here
      // that was not already done via restrictions on input
      return true;
		}

    /**
     * Configures and executes the LoadSassena algorithm.
     */
		void IndirectSassena::run()
    {
      using namespace Mantid::API;
      using MantidQt::API::BatchAlgorithmRunner;

			QString inputFileName = m_uiForm.mwInputFile->getFirstFilename();
			QFileInfo inputFileInfo(inputFileName);
      m_outWsName = inputFileInfo.baseName();
      bool save = m_uiForm.chkSave->isChecked();

      // If the workspace group already exists then remove it
      if(AnalysisDataService::Instance().doesExist(m_outWsName.toStdString()))
        AnalysisDataService::Instance().deepRemoveGroup(m_outWsName.toStdString());

      IAlgorithm_sptr sassenaAlg = AlgorithmManager::Instance().create("LoadSassena");
      sassenaAlg->initialize();

      sassenaAlg->setProperty("Filename", inputFileName.toStdString());
      sassenaAlg->setProperty("SortByQVectors", m_uiForm.cbSortQ->isChecked());
      sassenaAlg->setProperty("TimeUnit", m_uiForm.sbTimeUnit->value());
      sassenaAlg->setProperty("OutputWorkspace", m_outWsName.toStdString());

      m_batchAlgoRunner->addAlgorithm(sassenaAlg);

      BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromSassenaAlg;
      inputFromSassenaAlg["InputWorkspace"] = m_outWsName.toStdString();

      if(save)
      {
        QString saveFilename = m_outWsName + ".nxs";

        IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create("SaveNexus");
        saveAlg->initialize();

        saveAlg->setProperty("Filename", saveFilename.toStdString());

        m_batchAlgoRunner->addAlgorithm(saveAlg, inputFromSassenaAlg);
      }

      m_batchAlgoRunner->executeBatchAsync();
		}

    /**
     * Handles completion of the algorithm batch.
     *
     * @param error If the batch was stopped due to error
     */
    void IndirectSassena::handleAlgorithmFinish(bool error)
    {
      bool plot = m_uiForm.chkPlot->isChecked();

      // Nothing to do if the algorithm failed or we do not want to plot
      if(error || !plot)
        return;

      plotSpectrum(m_outWsName);
    }

    /**
     * Set the data selectors to use the default save directory
     * when browsing for input files.
     *
     * @param settings :: The settings to loading into the interface
     */
		void IndirectSassena::loadSettings(const QSettings& settings)
		{
			m_uiForm.mwInputFile->readSettings(settings.group());
		}

	} // namespace CustomInterfaces
} // namespace MantidQt
