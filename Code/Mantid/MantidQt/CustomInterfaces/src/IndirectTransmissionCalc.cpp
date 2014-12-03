#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/IndirectTransmissionCalc.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QTreeWidgetItem>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace
{
  Mantid::Kernel::Logger g_log("IndirectTransmissionCalc");
}

namespace MantidQt
{
  namespace CustomInterfaces
  {
    IndirectTransmissionCalc::IndirectTransmissionCalc(QWidget * parent) :
      IndirectToolsTab(parent)
    {
      m_uiForm.setupUi(parent);

      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
    }

    /*
     * Run any tab setup code.
     */
    void IndirectTransmissionCalc::setup()
    {
      instrumentSelected(m_uiForm.cbInstrument->currentText());

      connect(m_uiForm.cbInstrument, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(instrumentSelected(const QString&)));
      connect(m_uiForm.cbAnalyser, SIGNAL(currentIndexChanged(int)), this, SLOT(analyserSelected(int)));
    }

    /**
     * Validate the form to check the algorithm can be run.
     *
     * @return Whether the form was valid
     */
    bool IndirectTransmissionCalc::validate()
    {
      UserInputValidator uiv;

      uiv.checkFieldIsNotEmpty("Chemical Formula", m_uiForm.leChemicalFormula, m_uiForm.valChemicalFormula);

      // Ignore TOSCA and it's variants, they store efixed per detector
      std::string instrumentName = m_uiForm.cbInstrument->currentText().toStdString();
      if(instrumentName == "TOSCA" || instrumentName == "TFXA")
        uiv.addErrorMessage(QString::fromStdString(instrumentName) + " is currently not supported.");

      QString error = uiv.generateErrorMessage();
      showMessageBox(error);

      return error.isEmpty();
    }

    /**
     * Run the tab, invoking the IndirectTransmission algorithm.
     */
    void IndirectTransmissionCalc::run()
    {
      std::string instrumentName = m_uiForm.cbInstrument->currentText().toStdString();
      std::string outWsName = instrumentName + "_transmission";

      IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmission");
      transAlg->initialize();
      transAlg->setProperty("Instrument", instrumentName);
      transAlg->setProperty("Analyser", m_uiForm.cbAnalyser->currentText().toStdString());
      transAlg->setProperty("Reflection", m_uiForm.cbReflection->currentText().toStdString());
      transAlg->setProperty("ChemicalFormula", m_uiForm.leChemicalFormula->text().toStdString());
      transAlg->setProperty("NumberDensity", m_uiForm.spNumberDensity->value());
      transAlg->setProperty("Thickness", m_uiForm.spThickness->value());
      transAlg->setProperty("OutputWorkspace", outWsName);

      // Ensure completion signal is connected to correct slot
      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
      disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(instrumentLoadingDone(bool)));

      // Run the algorithm async
      runAlgorithm(transAlg);
    }

    /**
     * Handles completion of the IndirectTransmission algorithm.
     *
     * @param error If the algorithm encountered an error during execution
     */
    void IndirectTransmissionCalc::algorithmComplete(bool error)
    {
      enableInstrumentControls(true);

      if(error)
        return;

      std::string instrumentName = m_uiForm.cbInstrument->currentText().toStdString();
      std::string outWsName = instrumentName + "_transmission";

      ITableWorkspace_const_sptr resultTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWsName);
      Column_const_sptr propertyNames = resultTable->getColumn("Name");
      Column_const_sptr propertyValues = resultTable->getColumn("Value");

      // Update the table in the GUI
      m_uiForm.tvResultsTable->clear();

      for(size_t i = 0; i < resultTable->rowCount(); i++)
      {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString(propertyNames->cell<std::string>(i)));
        item->setText(1, QString::number(propertyValues->cell<double>(i)));
        m_uiForm.tvResultsTable->addTopLevelItem(item);
      }
    }

    /**
     * Set the file browser to use the default save directory
     * when browsing for input files.
     *
     * @param settings The settings to loading into the interface
     */
    void IndirectTransmissionCalc::loadSettings(const QSettings& settings)
    {
      UNUSED_ARG(settings);
    }

    /**
     * Handles an instrument being selected.
     *
     * Populates the analyser and reflection lists.
     *
     * @param instrumentName Name of selected instrument
     */
    void IndirectTransmissionCalc::instrumentSelected(const QString& instrumentName)
    {
      // Do not try to load the same instrument again
      if(instrumentName == m_instrument)
        return;

      // Disable the instrument controls while the instrument is being loaded
      enableInstrumentControls(false);

      // Remove the old empty instrument workspace if it is there
      std::string wsName = "__empty_" + m_instrument.toStdString();
      Mantid::API::AnalysisDataServiceImpl& dataStore = Mantid::API::AnalysisDataService::Instance();
      if(dataStore.doesExist(wsName))
        dataStore.remove(wsName);

      // Record the current instrument
      m_instrument = instrumentName;
      wsName = "__empty_" + m_instrument.toStdString();

      // Get the IDF file path form experiment info
      ExperimentInfo expInfo;
      std::string instFilename = expInfo.getInstrumentFilename(instrumentName.toStdString());

      if(instFilename.empty())
      {
        g_log.error("Could not find IDF for selected instrument");
        return;
      }

      // Load the instrument
      IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
      loadAlg->initialize();
      loadAlg->setProperty("Filename", instFilename);
      loadAlg->setProperty("OutputWorkspace", wsName);

      // Connect the completion signal to the instrument loading complete function
      disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(instrumentLoadingDone(bool)));

      // Run the algorithm async
      runAlgorithm(loadAlg);
    }

    void IndirectTransmissionCalc::instrumentLoadingDone(bool error)
    {
      // Switch the completion signal back to the original slot
      connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
      disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(instrumentLoadingDone(bool)));

      if(error)
      {
        g_log.error("Algorithm error when loading instrument");
        enableInstrumentControls(true);
        return;
      }

      std::string wsName = "__empty_" + m_instrument.toStdString();
      AnalysisDataServiceImpl& dataStore = AnalysisDataService::Instance();
      if(!dataStore.doesExist(wsName))
      {
        g_log.error("Instrument workspace was not found in ADS");
        enableInstrumentControls(true);
        return;
      }

      MatrixWorkspace_const_sptr instWorkspace = dataStore.retrieveWS<MatrixWorkspace>(wsName);
      Instrument_const_sptr instrument = instWorkspace->getInstrument();

      m_uiForm.cbAnalyser->clear();

      std::vector<std::string> analysers;
      boost::split(analysers, instrument->getStringParameter("analysers")[0], boost::is_any_of(","));

      for(auto it = analysers.begin(); it != analysers.end(); ++it)
      {
        std::string analyser = *it;
        std::string ipfReflections = instrument->getStringParameter("refl-" + analyser)[0];

        std::vector<std::string> reflections;
        boost::split(reflections, ipfReflections, boost::is_any_of(","), boost::token_compress_on);

        if(analyser != "diffraction") // Do not put diffraction into the analyser list
        {
          if(reflections.size() > 0)
          {
            QStringList reflectionsList;
            for(auto reflIt = reflections.begin(); reflIt != reflections.end(); ++reflIt)
              reflectionsList.push_back(QString::fromStdString(*reflIt));
            QVariant data = QVariant(reflectionsList);
            m_uiForm.cbAnalyser->addItem(QString::fromStdString(analyser), data);
          }
          else
          {
            m_uiForm.cbAnalyser->addItem(QString::fromStdString(analyser));
          }
        }
      }

      analyserSelected(m_uiForm.cbAnalyser->currentIndex());
    }

    /**
     * Handles an analyser being selected.
     *
     * Populates the reflection list.
     *
     * @param index Index of the selected analyser
     */
    void IndirectTransmissionCalc::analyserSelected(int index)
    {
      // Populate Reflection combobox with correct values for Analyser selected.
      m_uiForm.cbReflection->clear();

      QVariant currentData = m_uiForm.cbAnalyser->itemData(index);
      QStringList reflections = currentData.toStringList();
      for ( int i = 0; i < reflections.count(); i++ )
      {
        m_uiForm.cbReflection->addItem(reflections[i]);
      }

      enableInstrumentControls(true);
    }

    /**
     * Enable or disable the instrument setup controls.
     *
     * @param enabled If the controls should be enabled
     */
    void IndirectTransmissionCalc::enableInstrumentControls(bool enabled)
    {
      m_uiForm.cbInstrument->setEnabled(enabled);
      m_uiForm.cbAnalyser->setEnabled(enabled);
      m_uiForm.cbReflection->setEnabled(enabled);
    }

  } // namespace CustomInterfaces
} // namespace MantidQt
