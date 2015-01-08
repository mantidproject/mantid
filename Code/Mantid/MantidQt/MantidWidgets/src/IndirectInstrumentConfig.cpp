#include "MantidQtMantidWidgets/IndirectInstrumentConfig.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"


using MantidQt::MantidWidgets::InstrumentSelector;


namespace MantidQt
{
  namespace MantidWidgets
  {

    IndirectInstrumentConfig::IndirectInstrumentConfig(QWidget *parent, bool init): API::MantidWidget(parent),
      m_algRunner()
    {
      m_uiForm.setupUi(this);

      m_instrumentSelector = new InstrumentSelector(parent, init);
			m_uiForm.loInstrument->addWidget(m_instrumentSelector);
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


    void IndirectInstrumentConfig::updateInstrumentConfigurations()
    {
      //TODO
    }


  } /* namespace MantidWidgets */
} /* namespace MantidQt */
