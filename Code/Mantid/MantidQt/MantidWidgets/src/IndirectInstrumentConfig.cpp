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
      m_instrumentSelector->updateInstrumentOnSelection(false);
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


    /**
     * Gets the name of the facility instruments are displayed from.
     *
     * @return Name of facility
     */
    QString IndirectInstrumentConfig::getFacility()
    {
      return m_instrumentSelector->getFacility();
    }


    /**
     * Sets a facility to lock the widget to.
     *
     * @param facilityName Name of facility
     */
    void IndirectInstrumentConfig::setFacility(const QString & facilityName)
    {
      m_instrumentSelector->setAutoUpdate(false);
      m_instrumentSelector->setFacility(facilityName);
    }


    /**
     * Checks to see if diffraction is allowed in the analyser bank list.
     *
     * @return True if diffraction is an allowed analyser, false otherwise
     */
    bool IndirectInstrumentConfig::isDiffractionEnabled()
    {
      return !m_removeDiffraction;
    }


    /**
     * Sets if diffraction should bre removed from list of analyser banks.
     *
     * @param enabled Set to false to remove diffraction option
     */
    void IndirectInstrumentConfig::enableDiffraction(bool enabled)
    {
      if(!enabled)
        forceDiffraction(false);

      m_removeDiffraction = !enabled;
    }


    /**
     * Checks to see if diffraction is the only allowed analyser bank.
     *
     * @return True if diffraction is the only allowed analyser
     */
    bool IndirectInstrumentConfig::isDiffractionForced()
    {
      return m_forceDiffraction;
    }


    /**
     * Sets if diffraction should be the only allowed analyser bank option.
     *
     * @param forced If diffraction is the only allowed analyser
     */
    void IndirectInstrumentConfig::forceDiffraction(bool forced)
    {
      if(forced)
        enableDiffraction(true);

      m_forceDiffraction = forced;
    }


    /**
     * Sets the currently displayed instrument, providing that the name given
     * exists in the list currently displayed.
     *
     * @param instrumentName Name of instrument to display
     */
    void IndirectInstrumentConfig::setInstrument(const QString & instrumentName)
    {
      int index = m_instrumentSelector->findText(instrumentName);

      if(index >= 0)
      {
        m_instrumentSelector->setCurrentIndex(index);
      }
      else
      {
        g_log.information() << "Instrument " << instrumentName.toStdString()
                            << " not found in current list, using default" << std::endl;
      }
    }


    /**
     * Gets the name of the instrument that is currently selected.
     *
     * @return Name of instrument.
     */
    QString IndirectInstrumentConfig::getInstrumentName()
    {
      return m_instrumentSelector->currentText();
    }


    /**
     * Gets the name of the analyser bank that is currently selected.
     *
     * @return Name of analyser bank
     */
    QString IndirectInstrumentConfig::getAnalyserName()
    {
      return m_uiForm.cbAnalyser->currentText();
    }


    /**
     * Gets the name of the reflection mode currently selected.
     *
     * @return Name of reflection mode
     */
    QString IndirectInstrumentConfig::getReflectionName()
    {
      return m_uiForm.cbReflection->currentText();
    }


    /**
     * Updates the analyser and reflection names in the UI when an instrument is selected.
     *
     * @param instrumentName Nmae of instrument
     */
    void IndirectInstrumentConfig::updateInstrumentConfigurations(const QString & instrumentName)
    {
      if(instrumentName.isEmpty())
        return;

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

      std::vector<std::string> ipfAnalysers = instrument->getStringParameter("analysers");
      if(ipfAnalysers.size() == 0)
        return;

      QStringList analysers = QString::fromStdString(ipfAnalysers[0]).split(",");

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


    /**
     * Updates the list of reflection model when an analyser bank is selected.
     *
     * @param index Index of the analyser selected
     */
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

      newInstrumentConfiguration();
    }


    /**
     * Handles logging and signal emission when the instrument setup has been updated.
     *
     * Can be called manually to use instrumentConfigurationUpdated signal to init UI elements.
     */
    void IndirectInstrumentConfig::newInstrumentConfiguration()
    {
      g_log.debug() << "Instrument configuration: "
                    << "Instrument=" << getInstrumentName().toStdString()
                    << ", Analyser=" << getAnalyserName().toStdString()
                    << ", Reflection=" << getReflectionName().toStdString()
                    << std::endl;

      emit instrumentConfigurationUpdated(getInstrumentName(), getAnalyserName(), getReflectionName());
    }

  } /* namespace MantidWidgets */
} /* namespace MantidQt */
