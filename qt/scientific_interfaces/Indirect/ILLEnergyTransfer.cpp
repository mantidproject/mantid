// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ILLEnergyTransfer.h"

#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QFileInfo>
#include <QInputDialog>

using namespace Mantid::API;

namespace {

void saveNexusProcessed(std::string const &workspaceName,
                        std::string const &filename) {
  auto saver = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saver->initialize();
  saver->setProperty("InputWorkspace", workspaceName);
  saver->setProperty("Filename", filename);
  saver->execute();
}

} // namespace

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

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));

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

  // Validate map file if it is being used
  bool useMapFile = m_uiForm.rdGroupChoose->isChecked();
  if (useMapFile && !m_uiForm.rfMapFile->isValid())
    uiv.addErrorMessage("Grouping file is invalid.");

  // Validate background file
  if (!m_uiForm.rfBackgroundRun->isValid()) {
    uiv.addErrorMessage("Background Run File is invalid.");
  } else {
    bool isDouble = true;
    m_backScaling = m_uiForm.leBackgroundFactor->text().toDouble(&isDouble);
    if ((!isDouble || m_backScaling <= 0) &&
        !m_uiForm.rfBackgroundRun->getUserInput().toString().isEmpty()) {
      uiv.addErrorMessage("BackgroundScaleFactor is invalid. "
                          "It has to be a positive number.");
    }
  }

  // Validate calibration file
  if (!m_uiForm.rfCalibrationRun->isValid()) {
    uiv.addErrorMessage("Calibration Run File is invalid.");
  } else if (!m_uiForm.rfCalibrationRun->getUserInput().toString().isEmpty()) {
    auto range = m_uiForm.lePeakRange->text().split(',');
    if (range.size() != 2) {
      uiv.addErrorMessage("Calibration Peak Range is invalid. \n"
                          "Provide comma separated two energy values in meV.");
    } else {
      bool isDouble1 = true;
      m_peakRange[0] = range[0].toDouble(&isDouble1);
      bool isDouble2 = true;
      m_peakRange[1] = range[1].toDouble(&isDouble2);

      if (!isDouble1 || !isDouble2) {
        uiv.addErrorMessage(
            "Calibration Peak Range is invalid. \n"
            "Provide comma separated two energy values in meV.");
      } else {
        if (m_peakRange[0] >= m_peakRange[1]) {
          uiv.addErrorMessage("Calibration Peak Range is invalid. \n"
                              "Start energy is >= than the end energy.");
        }
      }
    }
  }

  // Validate the calibration background file
  if (!m_uiForm.rfBackCalibrationRun->isValid()) {
    uiv.addErrorMessage("Background run for calibration is invalid.");
  } else {
    bool isDouble = true;
    m_backCalibScaling = m_uiForm.leBackCalibScale->text().toDouble(&isDouble);
    if ((!isDouble || m_backCalibScaling <= 0) &&
        !m_uiForm.rfBackCalibrationRun->getUserInput().toString().isEmpty()) {
      uiv.addErrorMessage("Scale factor for calibration background is invalid. "
                          "It has to be a positive number.");
    }
  }

  // Calibration file required if calibration background is given
  if (!m_uiForm.rfBackCalibrationRun->getUserInput().toString().isEmpty() &&
      m_uiForm.rfCalibrationRun->getUserInput().toString().isEmpty()) {
    uiv.addErrorMessage(
        "Calibration file is required if calibration background is given");
  }

  // Validate the manual PSD integration range
  if (m_uiForm.rdGroupRange->isChecked()) {
    auto range = m_uiForm.lePixelRange->text().split(',');
    if (range.size() != 2) {
      uiv.addErrorMessage(
          "PSD Integration Range is invalid. \n"
          "Provide comma separated two pixel numbers, e.g. 1,128");
    } else {
      bool isDouble1 = true;
      m_pixelRange[0] = range[0].toInt(&isDouble1);
      bool isDouble2 = true;
      m_pixelRange[1] = range[1].toInt(&isDouble2);

      if (!isDouble1 || !isDouble2) {
        uiv.addErrorMessage(
            "PSD Integration Range is invalid. \n"
            "Provide comma separated two pixel numbers, e.g. 1,128");
      } else {
        if (m_pixelRange[0] >= m_pixelRange[1] || m_pixelRange[0] < 1 ||
            m_pixelRange[1] > 128) {
          uiv.addErrorMessage(
              "PSD Integration Range is invalid. \n"
              "Start or end pixel number is outside range [1-128], "
              "or start pixel number is >= than the end pixel number.");
        }
      }
    }
  }

  // Validate if the output workspace name is not empty
  if (m_uiForm.leOutWS->text().isEmpty())
    uiv.addErrorMessage("OutputWorkspace name is invalid.");

  // Validate QENS specific

  // Validate vanadium file if it is being used
  if (m_uiForm.rdQENS->isChecked()) {
    int useVanadiumRun = m_uiForm.sbUnmirrorOption->value();
    if ((useVanadiumRun == 5 || useVanadiumRun == 7) &&
        (!m_uiForm.rfAlignmentRun->isValid() ||
         m_uiForm.rfAlignmentRun->getUserInput().toString().isEmpty()))
      uiv.addErrorMessage("Alignment run is invalid.");
  }

  // Validate FWS specific

  if (m_uiForm.rdFWS->isChecked()) {
    if (m_uiForm.cbObservable->currentText().isEmpty()) {
      uiv.addErrorMessage("Observable is invalid, check the sample logs "
                          "for available options");
    }
  }

  // Show error message for errors
  if (!uiv.isAllInputValid())
    showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

void ILLEnergyTransfer::run() {
  QString runFilename = m_uiForm.rfInput->getUserInput().toString();
  QString backgroundFilename =
      m_uiForm.rfBackgroundRun->getUserInput().toString();
  QString calibrationFilename =
      m_uiForm.rfCalibrationRun->getUserInput().toString();
  QString calibrationBackgroundFilename =
      m_uiForm.rfBackCalibrationRun->getUserInput().toString();

  IAlgorithm_sptr reductionAlg = nullptr;

  if (m_uiForm.rdQENS->isChecked()) // QENS
  {
    reductionAlg =
        AlgorithmManager::Instance().create("IndirectILLReductionQENS");
    reductionAlg->initialize();

    // Set options
    long int uo = m_uiForm.sbUnmirrorOption->value();
    reductionAlg->setProperty("UnmirrorOption", uo);
    reductionAlg->setProperty("SumRuns", m_uiForm.ckSum->isChecked());
    reductionAlg->setProperty("CropDeadMonitorChannels",
                              m_uiForm.cbCrop->isChecked());

    // Calibraiton peak range
    if (!calibrationFilename.toStdString().empty()) {
      auto peakRange = boost::lexical_cast<std::string>(m_peakRange[0]) + "," +
                       boost::lexical_cast<std::string>(m_peakRange[1]);
      reductionAlg->setProperty("CalibrationPeakRange", peakRange);
    }

    // Vanadium alignment run
    if (uo == 5 || uo == 7) {
      QString vanFilename = m_uiForm.rfAlignmentRun->getUserInput().toString();
      reductionAlg->setProperty("AlignmentRun", vanFilename.toStdString());
    }

  } else { // FWS

    reductionAlg =
        AlgorithmManager::Instance().create("IndirectILLReductionFWS");
    reductionAlg->initialize();

    reductionAlg->setProperty(
        "Observable", m_uiForm.cbObservable->currentText().toStdString());

    reductionAlg->setProperty(
        "BackgroundOption", m_uiForm.cbBackOption->currentText().toStdString());

    reductionAlg->setProperty(
        "CalibrationOption",
        m_uiForm.cbCalibOption->currentText().toStdString());

    reductionAlg->setProperty(
        "CalibrationBackgroundOption",
        m_uiForm.cbBackCalibOption->currentText().toStdString());

    reductionAlg->setProperty("SortXAxis", m_uiForm.cbSortX->isChecked());
  }

  // options common for QENS and FWS

  // Handle input files
  reductionAlg->setProperty("Run", runFilename.toStdString());

  // Handle background file
  if (!backgroundFilename.toStdString().empty()) {
    reductionAlg->setProperty("BackgroundRun",
                              backgroundFilename.toStdString());
    reductionAlg->setProperty("BackgroundScalingFactor", m_backScaling);
  }

  // Handle calibration file
  if (!calibrationFilename.toStdString().empty()) {
    reductionAlg->setProperty("CalibrationRun",
                              calibrationFilename.toStdString());
  }

  // Handle calibration background file
  if (!calibrationBackgroundFilename.toStdString().empty()) {
    reductionAlg->setProperty("CalibrationBackgroundRun",
                              calibrationBackgroundFilename.toStdString());
    reductionAlg->setProperty("CalibrationBackgroundScalingFactor",
                              m_backCalibScaling);
  }

  reductionAlg->setProperty("Analyser",
                            getInstrumentDetail("analyser").toStdString());
  reductionAlg->setProperty("Reflection",
                            getInstrumentDetail("reflection").toStdString());

  std::string target = m_uiForm.cbSpectrumTarget->currentText().toStdString();
  reductionAlg->setProperty("SpectrumAxis", target);

  // Keep track of the suffix
  if (target == "SpectrumNumber") {
    m_suffix = "_red";
  } else if (target == "2Theta") {
    m_suffix = "_2theta";
  } else if (target == "Q") {
    m_suffix = "_q";
  } else if (target == "Q2") {
    m_suffix = "_q2";
  }

  // Handle mapping file
  bool useMapFile = m_uiForm.rdGroupChoose->isChecked();
  if (useMapFile) {
    QString mapFilename = m_uiForm.rfMapFile->getFirstFilename();
    reductionAlg->setProperty("MapFile", mapFilename.toStdString());
  }

  // Handle manual PSD integration range
  if (m_uiForm.rdGroupRange->isChecked()) {
    auto pixelRange = boost::lexical_cast<std::string>(m_pixelRange[0]) + "," +
                      boost::lexical_cast<std::string>(m_pixelRange[1]);
    reductionAlg->setProperty("ManualPSDIntegrationRange", pixelRange);
  }

  // Output workspace name
  QString outws = m_uiForm.leOutWS->text();
  reductionAlg->setProperty("OutputWorkspace", outws.toStdString());

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
}

/**
 * Handle when Run is clicked
 */
void ILLEnergyTransfer::runClicked() { runTab(); }

/**
 * Handles plotting of the reduced ws.
 */
void ILLEnergyTransfer::plot() {
  auto &ads = AnalysisDataService::Instance();

  auto const outputGroupName =
      m_uiForm.leOutWS->text().toStdString() + m_suffix;
  if (ads.doesExist(outputGroupName)) {
    auto const outputGroup = ads.retrieveWS<WorkspaceGroup>(outputGroupName);
    auto const workspaceName = outputGroup->getItem(0)->getName();
    IndirectTab::plot2D(QString::fromStdString(workspaceName));
  }
}

/**
 * Handles saving of the reduced ws.
 */
void ILLEnergyTransfer::save() {
  auto const workspaceName = m_uiForm.leOutWS->text().toStdString() + m_suffix;

  if (AnalysisDataService::Instance().doesExist(workspaceName))
    saveNexusProcessed(workspaceName, workspaceName + ".nxs");
}

/**
 * Called when the instrument has changed, used to update default values.
 */
void ILLEnergyTransfer::setInstrumentDefault() {
  auto const instrument = getInstrumentDetail("instrument");

  // Set instrument in run file widgets
  m_uiForm.rfInput->setInstrumentOverride(instrument);
  m_uiForm.rfMapFile->setInstrumentOverride(instrument);
}

void ILLEnergyTransfer::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void ILLEnergyTransfer::updateRunButton(bool enabled,
                                        std::string const &enableOutputButtons,
                                        QString const message,
                                        QString const tooltip) {
  UNUSED_ARG(enableOutputButtons);
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
}

} // namespace CustomInterfaces
} // namespace MantidQt
