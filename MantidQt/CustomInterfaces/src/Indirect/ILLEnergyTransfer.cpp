#include "MantidQtCustomInterfaces/Indirect/ILLEnergyTransfer.h"

#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>
#include <QInputDialog>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
ILLEnergyTransfer::ILLEnergyTransfer(IndirectDataReduction *idrUI,
                                     QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  connect(this, SIGNAL(newInstrumentConfiguration()), this,
          SLOT(setInstrumentDefault()));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));

  // Validate to remove invalid markers
  validateTab();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ILLEnergyTransfer::~ILLEnergyTransfer() {}

void ILLEnergyTransfer::setup() {}

bool ILLEnergyTransfer::validate() {
  UserInputValidator uiv;

  // Validate run file
  if (!m_uiForm.rfInput->isValid())
    uiv.addErrorMessage("Run File is invalid.");

  // Validate calibration file/workspace if it is being used
  if (m_uiForm.cbUseCalibration->isChecked())
    uiv.checkDataSelectorIsValid("Calibration", m_uiForm.dsCalibration);

  // Validate map file if it is being used
  bool useMapFile = m_uiForm.rdGroupChoose->isChecked();
  if (useMapFile && !m_uiForm.rfMapFile->isValid())
    uiv.addErrorMessage("Grouping file is invalid.");

  bool ok = true;
  m_backScaling = m_uiForm.leBackgroundFactor->text().toDouble(&ok);
  if (!ok || m_backScaling <= 0) {
    uiv.addErrorMessage("BackgroundScaleFactor is invalid.");
  }

  // Validate if the output workspace name is not empty
  if (m_uiForm.leOutWS->text().toStdString().empty())
    uiv.addErrorMessage("OutputWorkspace name is invalid.");

  // Validate QENS specific
  // Validate vanadium file if it is being used
  if (m_uiForm.rdQENS->isChecked()) {
  int useVanadiumRun = m_uiForm.sbUnmirrorOption->value();
  if ((useVanadiumRun == 5 || useVanadiumRun == 7) &&
      !m_uiForm.rfVanadiumRun->isValid())
    uiv.addErrorMessage("Vanadium run is invalid.");
  }

  // Validate FWS specific
  // Validate calibration file
  if (m_uiForm.rdFWS->isChecked()) {
    if (m_uiForm.cbUseCalibration->isChecked() &&
        m_uiForm.dsCalibration->isWorkspaceSelectorVisible()) {
      uiv.addErrorMessage("Please choose calibration File (not a workspace) "
                          "for FWS type of reduction.");
    }
    if (m_uiForm.cbObservable->currentText().toStdString().empty()) {
        uiv.addErrorMessage("Observable is invalid");
    }
  }

  // Show error message for errors
  if (!uiv.isAllInputValid())
    showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

void ILLEnergyTransfer::run() {
  QMap<QString, QString> instDetails = getInstrumentDetails();

  IAlgorithm_sptr reductionAlg = nullptr;

  bool useCalibration = m_uiForm.cbUseCalibration->isChecked();

  if (m_uiForm.rdQENS->isChecked()) // QENS
  {
    reductionAlg = AlgorithmManager::Instance().create("IndirectILLReduction");
    reductionAlg->initialize();

    // Set options
    long int uo = m_uiForm.sbUnmirrorOption->value();
    reductionAlg->setProperty("UnmirrorOption", uo);
    reductionAlg->setProperty("SumRuns", m_uiForm.ckSum->isChecked());

    // Handle calibration
    if (useCalibration) {
      std::string calibrationName;
      if (m_uiForm.dsCalibration->isWorkspaceSelectorVisible()) {
        calibrationName =
            m_uiForm.dsCalibration->getCurrentDataName().toStdString();
      } else {
        generateCalibrationWorkspace();
        calibrationName = "calib";
      }

      reductionAlg->setProperty("CalibrationWorkspace", calibrationName);
    }

    // Vanadium run
    if (uo == 5 || uo == 7) {
      QString vanFilename = m_uiForm.rfVanadiumRun->getUserInput().toString();
      reductionAlg->setProperty("VanadiumRun", vanFilename.toStdString());
    }

    // Handle background scaling factor
    reductionAlg->setProperty("BackgroundScalingFactor",m_backScaling);

  } else { // FWS

    reductionAlg =
        AlgorithmManager::Instance().create("IndirectILLFixedWindowScans");
    reductionAlg->initialize();

    // Handle calibration
    if (useCalibration) {
      if (m_uiForm.dsCalibration->isFileSelectorVisible()) {
        QString calibrationWsName =
            m_uiForm.dsCalibration->getCurrentDataName();
        reductionAlg->setProperty("CalibrationRun",
                                  calibrationWsName.toStdString());
      }
    }

    reductionAlg->setProperty(
        "Observable", m_uiForm.cbObservable->currentText().toStdString());

    reductionAlg->setProperty("SortXAxis", m_uiForm.cbSortX->isChecked());
  }

  // options common for QENS and FWS
  reductionAlg->setProperty("Analyser", instDetails["analyser"].toStdString());
  reductionAlg->setProperty("Reflection",
                            instDetails["reflection"].toStdString());

  reductionAlg->setProperty("DebugMode", m_uiForm.ckDebugMode->isChecked());

  // Handle input files
  QString runFilename = m_uiForm.rfInput->getUserInput().toString();
  reductionAlg->setProperty("Run", runFilename.toStdString());

  // Output workspace name
  QString outws = m_uiForm.leOutWS->text();
  reductionAlg->setProperty("OutputWorkspace", outws.toStdString());

  // Handle background file
  QString backgroundFilename = m_uiForm.rfBackgroundRun->getUserInput().toString();
  reductionAlg->setProperty("BackgroundRun", backgroundFilename.toStdString());

  // Handle mapping file
  bool useMapFile = m_uiForm.rdGroupChoose->isChecked();
  if (useMapFile) {
    QString mapFilename = m_uiForm.rfMapFile->getFirstFilename();
    reductionAlg->setProperty("MapFile", mapFilename.toStdString());
  }

  m_batchAlgoRunner->addAlgorithm(reductionAlg);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles completion of the algorithm.
 *
 * @param error True if the algorithm was stopped due to error, false otherwise
 */
void ILLEnergyTransfer::algorithmComplete(bool error) {
  if (error)
    return;
  else {
    if (m_uiForm.ckSave->isChecked()) {
      save();
    }
    if (m_uiForm.ckPlot->isChecked()) {
      plot();
    }
  }

  // Nothing to do here
}

/**
 * Handles plotting of the reduced ws.
 */
void ILLEnergyTransfer::plot() {
  QString pyInput = "from mantid import mtd\n"
                    "from IndirectReductionCommon import plot_reduction\n";
  pyInput += "plot_reduction(mtd[\"";
  pyInput += m_uiForm.leOutWS->text();
  pyInput += "\"].getItem(0).getName(),\"Contour\")\n";
  m_pythonRunner.runPythonCode(pyInput);
}

/**
 * Handles saving of the reduced ws.
 */
void ILLEnergyTransfer::save() {
  QString pyInput;
  pyInput += "SaveNexusProcessed(\"";
  pyInput += m_uiForm.leOutWS->text();
  pyInput += "\",\"";
  pyInput += m_uiForm.leOutWS->text();
  pyInput += ".nxs\")\n";
  m_pythonRunner.runPythonCode(pyInput);
}

/**
 * Runs ILLIN16BCalibration
 */
void ILLEnergyTransfer::generateCalibrationWorkspace() {
  QString pyInput;
  pyInput += "ILLIN16BCalibration(\"";
  pyInput += m_uiForm.dsCalibration->getCurrentDataName();
  pyInput += "\")\n";
  m_pythonRunner.runPythonCode(pyInput);
}


/**
 * Called when the instrument has changed, used to update default values.
 */
void ILLEnergyTransfer::setInstrumentDefault() {
  QMap<QString, QString> instDetails = getInstrumentDetails();

  // Set instrument in run file widgets
  m_uiForm.rfInput->setInstrumentOverride(instDetails["instrument"]);
  m_uiForm.rfMapFile->setInstrumentOverride(instDetails["instrument"]);
}

} // namespace CustomInterfaces
} // namespace Mantid
