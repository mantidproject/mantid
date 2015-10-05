#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/Indirect/CalculatePaalmanPings.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"

#include <QLineEdit>
#include <QList>
#include <QValidator>
#include <QDoubleValidator>
#include <QRegExpValidator>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("CalculatePaalmanPings");
}

namespace MantidQt {
namespace CustomInterfaces {
CalculatePaalmanPings::CalculatePaalmanPings(QWidget *parent)
    : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(getBeamWidthFromWorkspace(const QString &)));

  QRegExp regex("[A-Za-z0-9\\-\\(\\)]*");
  QValidator *formulaValidator = new QRegExpValidator(regex, this);
  m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
  m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);
}

void CalculatePaalmanPings::setup() { doValidation(true); }

void CalculatePaalmanPings::run() {
  // Get correct corrections algorithm
  QString sampleShape = m_uiForm.cbSampleShape->currentText();
  QString algorithmName =
      sampleShape.replace(" ", "") + "PaalmanPingsCorrection";
  algorithmName = algorithmName.replace(
      "Annulus", "Cylinder"); // Use the cylinder algorithm for annulus

  API::BatchAlgorithmRunner::AlgorithmRuntimeProps absCorProps;
  IAlgorithm_sptr absCorAlgo =
      AlgorithmManager::Instance().create(algorithmName.toStdString());
  absCorAlgo->initialize();

  // Sample details
  QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          sampleWsName.toStdString());

  // If not in wavelength then do conversion
  Mantid::Kernel::Unit_sptr sampleXUnit = sampleWs->getAxis(0)->unit();
  if (sampleXUnit->caption() != "Wavelength") {
    g_log.information(
        "Sample workspace not in wavelength, need to convert to continue.");
    absCorProps["SampleWorkspace"] =
        addConvertUnitsStep(sampleWs, "Wavelength");
  } else {
    absCorProps["SampleWorkspace"] = sampleWsName.toStdString();
  }

  double sampleNumberDensity = m_uiForm.spSampleNumberDensity->value();
  absCorAlgo->setProperty("SampleNumberDensity", sampleNumberDensity);

  QString sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
  absCorAlgo->setProperty("SampleChemicalFormula",
                          sampleChemicalFormula.toStdString());

  addShapeSpecificSampleOptions(absCorAlgo, sampleShape);

  // Can details
  bool useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {
    QString canWsName = m_uiForm.dsContainer->getCurrentDataName();
    MatrixWorkspace_sptr canWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            canWsName.toStdString());

    // If not in wavelength then do conversion
    Mantid::Kernel::Unit_sptr canXUnit = canWs->getAxis(0)->unit();
    if (canXUnit->caption() != "Wavelength") {
      g_log.information("Container workspace not in wavelength, need to "
                        "convert to continue.");
      absCorProps["CanWorkspace"] = addConvertUnitsStep(canWs, "Wavelength");
    } else {
      absCorProps["CanWorkspace"] = canWsName.toStdString();
    }

    double canNumberDensity = m_uiForm.spCanNumberDensity->value();
    absCorAlgo->setProperty("CanNumberDensity", canNumberDensity);

    QString canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
    absCorAlgo->setProperty("CanChemicalFormula",
                            canChemicalFormula.toStdString());

    addShapeSpecificCanOptions(absCorAlgo, sampleShape);
  }

  std::string eMode = getEMode(sampleWs);
  absCorAlgo->setProperty("EMode", eMode);
  if (eMode == "Indirect")
    absCorAlgo->setProperty("EFixed", getEFixed(sampleWs));

  // Generate workspace names
  int nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  QString correctionType;
  switch (m_uiForm.cbSampleShape->currentIndex()) {
  case 0:
    correctionType = "flt";
    break;
  case 1:
    correctionType = "cyl";
    break;
  case 2:
    correctionType = "ann";
    break;
  }

  const QString outputWsName =
      sampleWsName.left(nameCutIndex) + "_" + correctionType + "_abs";
  absCorAlgo->setProperty("OutputWorkspace", outputWsName.toStdString());

  // Add corrections algorithm to queue
  m_batchAlgoRunner->addAlgorithm(absCorAlgo, absCorProps);

  // Add save algorithms if required
  bool save = m_uiForm.ckSave->isChecked();
  if (save)
    addSaveWorkspaceToQueue(outputWsName);

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(absCorComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = outputWsName.toStdString();
}

bool CalculatePaalmanPings::validate() { return doValidation(); }

/**
 * Does validation on the user input.
 *
 * @param silent Set to true to avoid creating an error message
 * @return True if all user input is valid
 */
bool CalculatePaalmanPings::doValidation(bool silent) {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

  // Validate chemical formula
  if (uiv.checkFieldIsNotEmpty("Sample Chemical Formula",
                               m_uiForm.leSampleChemicalFormula,
                               m_uiForm.valSampleChemicalFormula))
    uiv.checkFieldIsValid("Sample Chemical Formula",
                          m_uiForm.leSampleChemicalFormula,
                          m_uiForm.valSampleChemicalFormula);

  bool useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {
    uiv.checkDataSelectorIsValid("Can", m_uiForm.dsContainer);

    // Validate chemical formula
    if (uiv.checkFieldIsNotEmpty("Can Chemical Formula",
                                 m_uiForm.leCanChemicalFormula,
                                 m_uiForm.valCanChemicalFormula))
      uiv.checkFieldIsValid("Can Chemical Formula",
                            m_uiForm.leCanChemicalFormula,
                            m_uiForm.valCanChemicalFormula);

    // Ensure sample and container are the same kind of data
    QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
    QString sampleType = sampleWsName.right(sampleWsName.length() -
                                            sampleWsName.lastIndexOf("_"));
    QString containerWsName = m_uiForm.dsContainer->getCurrentDataName();
    QString containerType = containerWsName.right(
        containerWsName.length() - containerWsName.lastIndexOf("_"));

    g_log.debug() << "Sample type is: " << sampleType.toStdString()
                  << std::endl;
    g_log.debug() << "Can type is: " << containerType.toStdString()
                  << std::endl;

    if (containerType != sampleType)
      uiv.addErrorMessage(
          "Sample and can workspaces must contain the same type of data.");
  }

  // Show error mssage if needed
  if (!uiv.isAllInputValid() && !silent)
    emit showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

/**
 * Handles completion of the correction algorithm.
 *
 * @param error True of the algorithm failed
 */
void CalculatePaalmanPings::absCorComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(absCorComplete(bool)));

  if (error) {
    emit showMessageBox("Absorption correction calculation failed.\nSee "
                        "Results Log for more details.");
    return;
  }

  // Convert the spectrum axis of correction factors to Q
  QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          sampleWsName.toStdString());
  WorkspaceGroup_sptr corrections =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          m_pythonExportWsName);
  for (size_t i = 0; i < corrections->size(); i++) {
    MatrixWorkspace_sptr factorWs =
        boost::dynamic_pointer_cast<MatrixWorkspace>(corrections->getItem(i));
    if (!factorWs || !sampleWs)
      continue;

    std::string eMode = getEMode(sampleWs);

    if (eMode == "Indirect") {
      API::BatchAlgorithmRunner::AlgorithmRuntimeProps convertSpecProps;
      IAlgorithm_sptr convertSpecAlgo =
          AlgorithmManager::Instance().create("ConvertSpectrumAxis");
      convertSpecAlgo->initialize();
      convertSpecAlgo->setProperty("InputWorkspace", factorWs);
      convertSpecAlgo->setProperty("OutputWorkspace", factorWs->name());
      convertSpecAlgo->setProperty("Target", "ElasticQ");
      convertSpecAlgo->setProperty("EMode", "Indirect");

      try {
        convertSpecAlgo->setProperty("EFixed", getEFixed(factorWs));
      } catch (std::runtime_error &) {
      }

      m_batchAlgoRunner->addAlgorithm(convertSpecAlgo);
    }
  }

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(postProcessComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles completion of the post processing algorithms.
 *
 * @param error True of the algorithm failed
 */
void CalculatePaalmanPings::postProcessComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(postProcessComplete(bool)));

  if (error) {
    emit showMessageBox("Correction factor post processing failed.\nSee "
                        "Results Log for more details.");
    return;
  }

  // Handle Mantid plotting
  QString plotType = m_uiForm.cbPlotOutput->currentText();

  if (plotType == "Both" || plotType == "Wavelength")
    plotSpectrum(QString::fromStdString(m_pythonExportWsName));

  if (plotType == "Both" || plotType == "Angle")
    plotTimeBin(QString::fromStdString(m_pythonExportWsName));
}

void CalculatePaalmanPings::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsContainer->readSettings(settings.group());
}

/**
 * Gets the beam width from the instrument parameters on a given workspace
 * and update the relevant options on the UI.
 *
 * @param wsName Name of the workspace
 */
void CalculatePaalmanPings::getBeamWidthFromWorkspace(const QString &wsName) {
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());

  if (!ws) {
    g_log.warning() << "Failed to find workspace " << wsName.toStdString()
                    << std::endl;
    return;
  }

  auto instrument = ws->getInstrument();

  const std::string beamWidthParamName = "Workflow.beam-width";
  if (instrument->hasParameter(beamWidthParamName)) {
    QString beamWidth = QString::fromStdString(
        instrument->getStringParameter(beamWidthParamName)[0]);
    double beamWidthValue = beamWidth.toDouble();

    m_uiForm.spCylBeamWidth->setValue(beamWidthValue);
    m_uiForm.spAnnBeamWidth->setValue(beamWidthValue);
  }

  const std::string beamHeightParamName = "Workflow.beam-height";
  if (instrument->hasParameter(beamHeightParamName)) {
    QString beamHeight = QString::fromStdString(
        instrument->getStringParameter(beamHeightParamName)[0]);
    double beamHeightValue = beamHeight.toDouble();

    m_uiForm.spCylBeamHeight->setValue(beamHeightValue);
    m_uiForm.spAnnBeamHeight->setValue(beamHeightValue);
  }
}

/**
 * Sets algorithm properties specific to the sample for a given shape.
 *
 * @param alg Algorithm to set properties of
 * @param shape Sample shape
 */
void CalculatePaalmanPings::addShapeSpecificSampleOptions(IAlgorithm_sptr alg,
                                                          QString shape) {
  if (shape == "FlatPlate") {
    double sampleThickness = m_uiForm.spFlatSampleThickness->value();
    alg->setProperty("SampleThickness", sampleThickness);

    double sampleAngle = m_uiForm.spFlatSampleAngle->value();
    alg->setProperty("SampleAngle", sampleAngle);
  } else if (shape == "Cylinder") {
    alg->setProperty("SampleInnerRadius", 0.0);

    double sampleOuterRadius = m_uiForm.spCylSampleOuterRadius->value();
    alg->setProperty("SampleOuterRadius", sampleOuterRadius);

    double beamWidth = m_uiForm.spCylBeamWidth->value();
    alg->setProperty("BeamWidth", beamWidth);

    double beamHeight = m_uiForm.spCylBeamHeight->value();
    alg->setProperty("BeamHeight", beamHeight);

    double stepSize = m_uiForm.spCylStepSize->value();
    alg->setProperty("StepSize", stepSize);
  } else if (shape == "Annulus") {
    double sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
    alg->setProperty("SampleInnerRadius", sampleInnerRadius);

    double sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
    alg->setProperty("SampleOuterRadius", sampleOuterRadius);

    double beamWidth = m_uiForm.spAnnBeamWidth->value();
    alg->setProperty("BeamWidth", beamWidth);

    double beamHeight = m_uiForm.spAnnBeamHeight->value();
    alg->setProperty("BeamHeight", beamHeight);

    double stepSize = m_uiForm.spAnnStepSize->value();
    alg->setProperty("StepSize", stepSize);
  }
}

/**
 * Sets algorithm properties specific to the container for a given shape.
 *
 * @param alg Algorithm to set properties of
 * @param shape Sample shape
 */
void CalculatePaalmanPings::addShapeSpecificCanOptions(IAlgorithm_sptr alg,
                                                       QString shape) {
  if (shape == "FlatPlate") {
    double canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
    alg->setProperty("CanFrontThickness", canFrontThickness);

    double canBackThickness = m_uiForm.spFlatCanBackThickness->value();
    alg->setProperty("SampleThickness", canBackThickness);
  } else if (shape == "Cylinder") {
    double canOuterRadius = m_uiForm.spCylCanOuterRadius->value();
    alg->setProperty("CanOuterRadius", canOuterRadius);
  } else if (shape == "Annulus") {
    double canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
    alg->setProperty("CanOuterRadius", canOuterRadius);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
