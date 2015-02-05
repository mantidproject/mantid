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
      QRegExp chemicalFormulaRegex("[A-Za-z0-9\\-\\(\\)]*");
      QValidator *chemicalFormulaValidator = new QRegExpValidator(chemicalFormulaRegex, this);
      m_uiForm.leChemicalFormula->setValidator(chemicalFormulaValidator);
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

      QString error = uiv.generateErrorMessage();
      showMessageBox(error);

      return error.isEmpty();
    }


    /**
     * Run the tab, invoking the IndirectTransmission algorithm.
     */
    void IndirectTransmissionCalc::run()
    {
      std::string instrumentName = m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString();
      std::string outWsName = instrumentName + "_transmission";

      IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmission");
      transAlg->initialize();
      transAlg->setProperty("Instrument", instrumentName);
      transAlg->setProperty("Analyser", m_uiForm.iicInstrumentConfiguration->getAnalyserName().toStdString());
      transAlg->setProperty("Reflection", m_uiForm.iicInstrumentConfiguration->getReflectionName().toStdString());
      transAlg->setProperty("ChemicalFormula", m_uiForm.leChemicalFormula->text().toStdString());
      transAlg->setProperty("NumberDensity", m_uiForm.spNumberDensity->value());
      transAlg->setProperty("Thickness", m_uiForm.spThickness->value());
      transAlg->setProperty("OutputWorkspace", outWsName);

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
      if(error)
      {
        emit showMessageBox("Failed to execute IndirectTransmission algorithm.\nSee Results Log for details.");
        return;
      }

      std::string instrumentName = m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString();
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

  } // namespace CustomInterfaces
} // namespace MantidQt
