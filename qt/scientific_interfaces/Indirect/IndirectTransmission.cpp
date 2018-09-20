#include "IndirectTransmission.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QFileInfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectTransmission::IndirectTransmission(IndirectDataReduction *idrUI,
                                           QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  connect(this, SIGNAL(newInstrumentConfiguration()), this,
          SLOT(instrumentSet()));

  // Update the preview plot when the algorithm is complete
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(transAlgDone(bool)));
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(QString)), this,
          SLOT(dataLoaded()));
  connect(m_uiForm.dsCanInput, SIGNAL(dataReady(QString)), this,
          SLOT(dataLoaded()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectTransmission::~IndirectTransmission() {}

void IndirectTransmission::setup() {}

void IndirectTransmission::run() {
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
  QString outWsName = sampleWsName + "_transmission";

  IAlgorithm_sptr transAlg =
      AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
  transAlg->initialize();

  transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
  transAlg->setProperty("CanWorkspace", canWsName.toStdString());
  transAlg->setProperty("OutputWorkspace", outWsName.toStdString());

  m_batchAlgoRunner->addAlgorithm(transAlg);
  m_batchAlgoRunner->executeBatchAsync();
}

bool IndirectTransmission::validate() {
  // Check if we have an appropriate instrument
  QString currentInst = getInstrumentConfiguration()->getInstrumentName();
  if (currentInst != "IRIS" && currentInst != "OSIRIS")
    return false;

  // Check for an invalid sample input
  if (!m_uiForm.dsSampleInput->isValid())
    return false;

  // Check for an invalid can input
  if (!m_uiForm.dsCanInput->isValid())
    return false;

  return true;
}

void IndirectTransmission::dataLoaded() {
  if (validate())
    previewPlot();
}

void IndirectTransmission::previewPlot() {
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
  QString outWsName = sampleWsName + "_transmission";

  IAlgorithm_sptr transAlg =
      AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
  transAlg->initialize();

  transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
  transAlg->setProperty("CanWorkspace", canWsName.toStdString());
  transAlg->setProperty("OutputWorkspace", outWsName.toStdString());

  // Set the workspace name for Python script export
  m_pythonExportWsName = sampleWsName.toStdString() + "_Trans";

  runAlgorithm(transAlg);
}

void IndirectTransmission::transAlgDone(bool error) {
  if (error)
    return;

  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  QString outWsName = sampleWsName + "_transmission";

  WorkspaceGroup_sptr resultWsGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          outWsName.toStdString());

  // Do plotting
  m_uiForm.ppPlot->clear();
  m_uiForm.ppPlot->addSpectrum("Can", sampleWsName + "_Can", 0, Qt::black);
  m_uiForm.ppPlot->addSpectrum("Sample", sampleWsName + "_Sam", 0, Qt::red);
  m_uiForm.ppPlot->addSpectrum("Transmission", sampleWsName + "_Trans", 0,
                               Qt::blue);
  m_uiForm.ppPlot->resizeX();

  // Enable plot and save
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

void IndirectTransmission::instrumentSet() {
  QMap<QString, QString> instDetails = getInstrumentDetails();

  // Set the search instrument for runs
  m_uiForm.dsSampleInput->setInstrumentOverride(instDetails["instrument"]);
  m_uiForm.dsCanInput->setInstrumentOverride(instDetails["instrument"]);
}

/**
 * Handle saving of workspace
 */
void IndirectTransmission::saveClicked() {
  QString outputWs =
      (m_uiForm.dsSampleInput->getCurrentDataName() + "_transmission");

  if (checkADSForPlotSaveWorkspace(outputWs.toStdString(), false))
    addSaveWorkspaceToQueue(outputWs);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle mantid plotting
 */
void IndirectTransmission::plotClicked() {
  QString outputWs =
      (m_uiForm.dsSampleInput->getCurrentDataName() + "_transmission");
  if (checkADSForPlotSaveWorkspace(outputWs.toStdString(), true))
    plotSpectrum(outputWs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
