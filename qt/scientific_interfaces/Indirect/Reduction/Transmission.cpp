// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Transmission.h"

#include "MantidAPI/WorkspaceGroup.h"
#include "ReductionAlgorithmUtils.h"

#include <MantidQtWidgets/Common/ParseKeyValueString.h>
#include <QFileInfo>

using namespace Mantid::API;

namespace {

void conjoinSpectra(std::string const &inputWorkspaces, std::string const &outputName) {
  auto conjoin = AlgorithmManager::Instance().create("ConjoinSpectra");
  conjoin->initialize();
  conjoin->setProperty("InputWorkspaces", inputWorkspaces);
  conjoin->setProperty("OutputWorkspace", outputName);
  conjoin->execute();
}

} // namespace

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
Transmission::Transmission(IDataReduction *idrUI, QWidget *parent) : DataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  setOutputPlotOptionsPresenter(m_uiForm.ipoPlotOptions, PlotWidget::Spectra, "0-2");

  // Update the preview plot when the algorithm is complete
  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &Transmission::transAlgDone);

  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &Transmission::saveClicked);
  connect(m_uiForm.dsSampleInput, &FileFinderWidget::filesFoundChanged, this,
          [this]() { Transmission::handleNewInputData(SenderType::sampleInput); });
  connect(m_uiForm.dsCanInput, &FileFinderWidget::filesFoundChanged, this,
          [this]() { Transmission::handleNewInputData(SenderType::canInput); });
  connect(m_uiForm.ckSumFiles, &QCheckBox::stateChanged, this,
          [this]() { Transmission::handleNewInputData(SenderType::sumCheckbox); });

  m_uiForm.ppPlot->setCanvasColour(QColor(240, 240, 240));
}

Transmission::~Transmission() = default;

QString Transmission::loadFiles(const QStringList &fileNames) {
  if (fileNames.empty()) {
    return "";
  }

  QString wsname;
  bool loadError = false;
  if (!m_uiForm.ckSumFiles->isChecked() || fileNames.size() == 1) {
    const QString fileName = fileNames.at(0);
    const QFileInfo fi(fileName);
    wsname = fi.baseName();
    loadError = !loadFile(fileName.toStdString(), wsname.toStdString());
  } else {
    wsname =
        QString::fromStdString(loadFilesWithSum(MantidWidgets::qStringListToStdVector(fileNames), getIpfFilename()));
    loadError = wsname.isEmpty();
  }

  if (loadError) {
    emit showMessageBox("Unable to load file.\nCheck whether your file exists "
                        "and matches the selected instrument in the "
                        "EnergyTransfer tab.");
    wsname = "";
  }
  return wsname;
}

void Transmission::handleNewInputData(const SenderType &senderType) {
  switch (senderType) {
  case SenderType::sampleInput: {
    const auto sampleName = loadFiles(m_uiForm.dsSampleInput->getFilenames());
    m_sampleName = sampleName;
    break;
  }
  case SenderType::canInput: {
    const auto canName = loadFiles(m_uiForm.dsCanInput->getFilenames());
    m_canName = canName;
    break;
  }
  case SenderType::sumCheckbox: {
    const auto canName = loadFiles(m_uiForm.dsCanInput->getFilenames());
    const auto sampleName = loadFiles(m_uiForm.dsSampleInput->getFilenames());
    m_sampleName = sampleName;
    m_canName = canName;
    break;
  }
  }
}

void Transmission::handleRun() {
  const auto outWsName = m_sampleName.toLower().toStdString() + "_transmission_group";
  const auto transAlg = AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
  transAlg->initialize();

  transAlg->setProperty("SampleWorkspace", m_sampleName.toStdString());
  transAlg->setProperty("CanWorkspace", m_canName.toStdString());
  transAlg->setProperty("OutputWorkspace", outWsName);

  m_batchAlgoRunner->addAlgorithm(transAlg);
  m_batchAlgoRunner->executeBatchAsync();

  m_pythonExportWsName = outWsName;
}

void Transmission::handleValidation(IUserInputValidator *validator) const {
  // Check if we have an appropriate instrument
  QString currentInst = getInstrumentName();
  if (currentInst != "IRIS" && currentInst != "OSIRIS")
    validator->addErrorMessage("The selected instrument must be IRIS or OSIRIS");

  validator->checkFileFinderWidgetIsValid("Sample", m_uiForm.dsSampleInput);
  validator->checkFileFinderWidgetIsValid("Can", m_uiForm.dsCanInput);
}

void Transmission::transAlgDone(bool error) {
  m_runPresenter->setRunEnabled(true);
  m_uiForm.pbSave->setEnabled(!error);
  if (error)
    return;

  auto const sampleWsName = m_sampleName;
  auto const transmissionName = sampleWsName.toLower().toStdString() + "_transmission";
  conjoinSpectra(sampleWsName.toStdString() + "_Can," + sampleWsName.toStdString() + "_Sam," +
                     sampleWsName.toStdString() + "_Trans",
                 transmissionName);

  setOutputPlotOptionsWorkspaces({transmissionName});

  // Do plotting
  m_uiForm.ppPlot->clear();
  m_uiForm.ppPlot->addSpectrum("Can", sampleWsName + "_Can", 0, Qt::black);
  m_uiForm.ppPlot->addSpectrum("Sample", sampleWsName + "_Sam", 0, Qt::red);
  m_uiForm.ppPlot->addSpectrum("Transmission", sampleWsName + "_Trans", 0, Qt::blue);
  m_uiForm.ppPlot->resizeX();
}

void Transmission::updateInstrumentConfiguration() {
  try {
    setInstrument(getInstrumentDetail("instrument"));
  } catch (std::exception const &ex) {
    showMessageBox(ex.what());
  }
}

void Transmission::setInstrument(QString const &instrumentName) {
  m_uiForm.dsSampleInput->setInstrumentOverride(instrumentName);
  m_uiForm.dsCanInput->setInstrumentOverride(instrumentName);
}

/**
 * Handle saving of workspace
 */
void Transmission::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(m_pythonExportWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void Transmission::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

} // namespace MantidQt::CustomInterfaces
