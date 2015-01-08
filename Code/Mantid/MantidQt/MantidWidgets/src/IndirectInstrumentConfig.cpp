#include "MantidQtMantidWidgets/IndirectInstrumentConfig.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"


using namespace Mantid::API;
using namespace Mantid::Geometry;
using MantidQt::MantidWidgets::InstrumentSelector;


namespace
{
  Mantid::Kernel::Logger g_log("IndirectInstrumentConfig");
}


namespace MantidQt
{
  namespace MantidWidgets
  {

    IndirectInstrumentConfig::IndirectInstrumentConfig(QWidget *parent, bool init): API::MantidWidget(parent),
      m_algRunner()
    {
      m_uiForm.setupUi(this);

      m_instrumentSelector = new InstrumentSelector(0, init);
			m_uiForm.loInstrument->addWidget(m_instrumentSelector);

      connect(m_instrumentSelector, SIGNAL(instrumentSelectionChanged(const QString)),
              this, SLOT(updateInstrumentConfigurations(const QString)));
      connect(m_uiForm.cbAnalyser, SIGNAL(currentIndexChanged(int)),
              this, SLOT(updateReflectionsList(int)));
      connect(m_uiForm.cbReflection, SIGNAL(currentIndexChanged(int)),
              this, SLOT(newInstrumentConfiguration()));
    }

    IndirectInstrumentConfig::~IndirectInstrumentConfig()
    {
    }

    /**
     * Gets the list of techniques used to filter instruments by.
     *
     * @return List of techniques
     */
    QStringList IndirectInstrumentConfig::getTechniques()
    {
      return m_instrumentSelector->getTechniques();
    }

    /**
     * Set a list of techniques by which the list of instruments should be filtered.
     *
     * @param techniques List of techniques
     */
    void IndirectInstrumentConfig::setTechniques(const QStringList & techniques)
    {
      m_instrumentSelector->setTechniques(techniques);
    }


    QStringList IndirectInstrumentConfig::getDisabledInstruments()
    {
      return m_disabledInstruments;
    }


    void IndirectInstrumentConfig::setDisabledInstruments(const QStringList & instrumentNames)
    {
      //TODO
    }


    QString IndirectInstrumentConfig::getFacility()
    {
      return m_instrumentSelector->getFacility();
    }


    void IndirectInstrumentConfig::setFacility(const QString & facilityName)
    {
      m_instrumentSelector->setAutoUpdate(false);
      m_instrumentSelector->setFacility(facilityName);
    }


    bool IndirectInstrumentConfig::isDiffractionEnabled()
    {
      return !m_removeDiffraction;
    }


    void IndirectInstrumentConfig::enableDiffraction(bool enabled)
    {
      if(!enabled)
        forceDiffraction(false);

      m_removeDiffraction = !enabled;
    }


    bool IndirectInstrumentConfig::isDiffractionForced()
    {
      return m_forceDiffraction;
    }


    void IndirectInstrumentConfig::forceDiffraction(bool forced)
    {
      if(forced)
        enableDiffraction(true);

      m_forceDiffraction = forced;
    }


    bool IndirectInstrumentConfig::willAutoLoadConfigurations()
    {
      return m_autoLoad;
    }


    void IndirectInstrumentConfig::autoLoadConfigurations(bool autoLoad)
    {
      m_autoLoad = autoLoad;
    }


    QString IndirectInstrumentConfig::getInstrumentName()
    {
      return m_instrumentSelector->currentText();
    }


    QString IndirectInstrumentConfig::getAnalyserName()
    {
      return m_uiForm.cbAnalyser->currentText();
    }


    QString IndirectInstrumentConfig::getReflectionName()
    {
      return m_uiForm.cbReflection->currentText();
    }


    void IndirectInstrumentConfig::updateInstrumentConfigurations(const QString & instrumentName)
    {
      g_log.debug() << "Loading configuration for instrument: " << instrumentName.toStdString() << std::endl;

      bool analyserPreviousBlocking = m_uiForm.cbAnalyser->signalsBlocked();
      m_uiForm.cbAnalyser->blockSignals(true);

      m_uiForm.cbAnalyser->clear();

      IAlgorithm_sptr loadInstAlg = AlgorithmManager::Instance().create("CreateSimulationWorkspace");
      loadInstAlg->initialize();
      loadInstAlg->setChild(true);
      loadInstAlg->setProperty("Instrument", instrumentName.toStdString());
      loadInstAlg->setProperty("BinParams", "0,0.5,1");
      loadInstAlg->setProperty("OutputWorkspace", "__empty_instrument_workspace");
      loadInstAlg->execute();
      MatrixWorkspace_sptr instWorkspace = loadInstAlg->getProperty("OutputWorkspace");

      QList<QPair<QString, QString>> instrumentModes;
      Instrument_const_sptr instrument = instWorkspace->getInstrument();

      std::string ipfAnalysers = instrument->getStringParameter("analysers")[0];
      QStringList analysers = QString::fromStdString(ipfAnalysers).split(",");

      for(auto it = analysers.begin(); it != analysers.end(); ++it)
      {
        QString analyser = *it;
        std::string ipfReflections = instrument->getStringParameter("refl-" + analyser.toStdString())[0];
        QStringList reflections = QString::fromStdString(ipfReflections).split(",");

        if(m_removeDiffraction && analyser == "diffraction")
          continue;

        if(m_forceDiffraction && analyser != "diffraction")
          continue;

        if(reflections.size() > 0)
        {
          QVariant data = QVariant(reflections);
          m_uiForm.cbAnalyser->addItem(analyser, data);
        }
        else
        {
          m_uiForm.cbAnalyser->addItem(analyser);
        }
      }

      int index = m_uiForm.cbAnalyser->currentIndex();
      updateReflectionsList(index);

      m_uiForm.cbAnalyser->blockSignals(analyserPreviousBlocking);
    }


    void IndirectInstrumentConfig::updateReflectionsList(int index)
    {
      bool reflectionPreviousBlocking = m_uiForm.cbReflection->signalsBlocked();
      m_uiForm.cbReflection->blockSignals(true);

      m_uiForm.cbReflection->clear();

      QVariant currentData = m_uiForm.cbAnalyser->itemData(index);
      bool valid = currentData != QVariant::Invalid;
      m_uiForm.cbReflection->setEnabled(valid);

      if(valid)
      {
        QStringList reflections = currentData.toStringList();
        for ( int i = 0; i < reflections.count(); i++ )
          m_uiForm.cbReflection->addItem(reflections[i]);
      }
      else
      {
        m_uiForm.cbReflection->addItem("No Valid Reflections");
      }

      m_uiForm.cbReflection->blockSignals(reflectionPreviousBlocking);
    }


    void IndirectInstrumentConfig::newInstrumentConfiguration()
    {
      g_log.information() << "Instrument configuration: "
                          << "Instrument=" << getInstrumentName().toStdString()
                          << ", Analyser=" << getAnalyserName().toStdString()
                          << ", Reflection=" << getReflectionName().toStdString()
                          << std::endl;

      emit instrumentConfigurationUpdated(getInstrumentName(), getAnalyserName(), getReflectionName());
    }

  } /* namespace MantidWidgets */
} /* namespace MantidQt */
