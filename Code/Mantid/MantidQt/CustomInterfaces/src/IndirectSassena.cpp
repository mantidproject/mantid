#include "MantidQtCustomInterfaces/IndirectSassena.h"

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

			QString inputFileName = m_uiForm.mwInputFile->getFirstFilename();
			QFileInfo inputFileInfo(inputFileName);
      QString outWsName = inputFileInfo.baseName();
      bool save = m_uiForm.chkSave->isChecked();

      IAlgorithm_sptr sassenaAlg = AlgorithmManager::Instance().create("LoadSassena");
      sassenaAlg->initialize();

      sassenaAlg->setProperty("Filename", inputFileName.toStdString());
      sassenaAlg->setProperty("SortByQVectors", m_uiForm.cbSortQ->isChecked());
      sassenaAlg->setProperty("TimeUnit", m_uiForm.sbTimeUnit->value());
      sassenaAlg->setProperty("OutputWorkspace", outWsName.toStdString());

      sassenaAlg->execute();
      /* runAlgorithm(sassenaAlg); */

      if(save)
      {
        QString saveFilename = outWsName + ".nxs";

        IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create("SaveNexus");
        saveAlg->initialize();

        saveAlg->setProperty("InputWorkspace", outWsName.toStdString());
        saveAlg->setProperty("Filename", saveFilename.toStdString());

        saveAlg->execute();
      }
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
