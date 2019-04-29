// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectTransmissionCalc.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QTreeWidgetItem>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
Mantid::Kernel::Logger g_log("IndirectTransmissionCalc");
}

namespace MantidQt {
namespace CustomInterfaces {
IndirectTransmissionCalc::IndirectTransmissionCalc(QWidget *parent)
    : IndirectToolsTab(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
}

/*
 * Run any tab setup code.
 */
void IndirectTransmissionCalc::setup() {
  QRegExp chemicalFormulaRegex(R"([A-Za-z0-9\-\(\)]*)");
  QValidator *chemicalFormulaValidator =
      new QRegExpValidator(chemicalFormulaRegex, this);
  m_uiForm.leChemicalFormula->setValidator(chemicalFormulaValidator);
}

/**
 * Validate the form to check the algorithm can be run.
 *
 * @return Whether the form was valid
 */
bool IndirectTransmissionCalc::validate() {
  UserInputValidator uiv;

  uiv.checkFieldIsNotEmpty("Chemical Formula", m_uiForm.leChemicalFormula,
                           m_uiForm.valChemicalFormula);

  QString error = uiv.generateErrorMessage();
  showMessageBox(error);

  return error.isEmpty();
}

/**
 * Run the tab, invoking the IndirectTransmission algorithm.
 */
void IndirectTransmissionCalc::run() {
  setRunIsRunning(true);

  std::string instrumentName =
      m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString();
  std::string outWsName = instrumentName + "_transmission";

  IAlgorithm_sptr transAlg =
      AlgorithmManager::Instance().create("IndirectTransmission");
  transAlg->initialize();
  transAlg->setProperty("Instrument", instrumentName);
  transAlg->setProperty(
      "Analyser",
      m_uiForm.iicInstrumentConfiguration->getAnalyserName().toStdString());
  transAlg->setProperty(
      "Reflection",
      m_uiForm.iicInstrumentConfiguration->getReflectionName().toStdString());
  transAlg->setProperty("ChemicalFormula",
                        m_uiForm.leChemicalFormula->text().toStdString());
  if (m_uiForm.cbDensityType->currentIndex() == 0)
    transAlg->setProperty("DensityType", "Mass Density");
  else
    transAlg->setProperty("DensityType", "Number Density");
  transAlg->setProperty("Density", m_uiForm.spDensity->value());
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
void IndirectTransmissionCalc::algorithmComplete(bool error) {
  setRunIsRunning(false);

  if (!error) {
    std::string const instrumentName =
        m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString();
    std::string const outWsName = instrumentName + "_transmission";

    auto resultTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWsName);
    Column_const_sptr propertyNames = resultTable->getColumn("Name");
    Column_const_sptr propertyValues = resultTable->getColumn("Value");

    // Update the table in the GUI
    m_uiForm.tvResultsTable->clear();

    for (auto i = 0u; i < resultTable->rowCount(); ++i) {
      QTreeWidgetItem *item = new QTreeWidgetItem();
      item->setText(
          0, QString::fromStdString(propertyNames->cell<std::string>(i)));
      item->setText(1, QString::number(propertyValues->cell<double>(i)));
      m_uiForm.tvResultsTable->addTopLevelItem(item);
    }
  } else
    emit showMessageBox("Failed to execute IndirectTransmission "
                        "algorithm.\nSee Results Log for details.");
}

/**
 * Set the file browser to use the default save directory
 * when browsing for input files.
 *
 * @param settings The settings to loading into the interface
 */
void IndirectTransmissionCalc::loadSettings(const QSettings &settings) {
  UNUSED_ARG(settings);
}

void IndirectTransmissionCalc::runClicked() { runTab(); }

void IndirectTransmissionCalc::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setRunEnabled(!running);
}

void IndirectTransmissionCalc::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

} // namespace CustomInterfaces
} // namespace MantidQt
