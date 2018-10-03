// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "CalculatePaalmanPings.h"
#include "../General/UserInputValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/Common/WorkspaceSelector.h"

#include <QDoubleValidator>
#include <QLineEdit>
#include <QList>
#include <QRegExpValidator>
#include <QValidator>

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

  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(fillCorrectionDetails(const QString &)));

  QRegExp regex(R"([A-Za-z0-9\-\(\)]*)");
  QValidator *formulaValidator = new QRegExpValidator(regex, this);
  m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
  m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);
  connect(m_uiForm.leSampleChemicalFormula, SIGNAL(editingFinished()), this,
          SLOT(validateChemical()));
  connect(m_uiForm.leCanChemicalFormula, SIGNAL(editingFinished()), this,
          SLOT(validateChemical()));
  // Connect slots for plot and save
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));

  // Connect slots for toggling the mass/number density unit
  connect(m_uiForm.cbSampleDensity, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeSampleDensityUnit(int)));
  connect(m_uiForm.cbCanDensity, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeCanDensityUnit(int)));

  UserInputValidator uiv;
  if (uiv.checkFieldIsNotEmpty("Can Chemical Formula",
                               m_uiForm.leCanChemicalFormula,
                               m_uiForm.valCanChemicalFormula)) {
    uiv.checkFieldIsValid("Can Chemical Formula", m_uiForm.leCanChemicalFormula,
                          m_uiForm.valCanChemicalFormula);
  }
}

void CalculatePaalmanPings::setup() { doValidation(true); }

void CalculatePaalmanPings::validateChemical() { doValidation(true); }

void CalculatePaalmanPings::run() {
  // Get correct corrections algorithm
  auto sampleShape = m_uiForm.cbSampleShape->currentText();
  auto algorithmName = sampleShape.replace(" ", "") + "PaalmanPingsCorrection";
  algorithmName = algorithmName.replace(
      "Annulus", "Cylinder"); // Use the cylinder algorithm for annulus

  API::BatchAlgorithmRunner::AlgorithmRuntimeProps absCorProps;
  auto absCorAlgo =
      AlgorithmManager::Instance().create(algorithmName.toStdString());
  absCorAlgo->initialize();

  // Sample details
  auto sampleWsName = m_uiForm.dsSample->getCurrentDataName();
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          sampleWsName.toStdString());

  const auto emode = m_uiForm.cbEmode->currentText();
  absCorAlgo->setProperty("EMode", emode.toStdString());

  const auto efixed = m_uiForm.doubleEfixed->value();
  absCorAlgo->setProperty("EFixed", efixed);

  const long int numwave = m_uiForm.spNwave->value();
  absCorAlgo->setProperty("NumberWavelengths", numwave);

  const bool inter = m_uiForm.cbInterpolate->isChecked();
  absCorAlgo->setProperty("Interpolate", inter);

  // If not in wavelength then do conversion
  const auto sampleXUnit = sampleWs->getAxis(0)->unit();
  if (sampleXUnit->caption() != "Wavelength" && emode != "Efixed") {
    g_log.information(
        "Sample workspace not in wavelength, need to convert to continue.");
    absCorProps["SampleWorkspace"] =
        addConvertUnitsStep(sampleWs, "Wavelength");
  } else {
    absCorProps["SampleWorkspace"] = sampleWsName.toStdString();
  }

  absCorAlgo->setProperty(
      "SampleDensityType",
      m_uiForm.cbSampleDensity->currentText().toStdString());
  absCorAlgo->setProperty("SampleDensity", m_uiForm.spSampleDensity->value());

  absCorAlgo->setProperty(
      "SampleChemicalFormula",
      m_uiForm.leSampleChemicalFormula->text().toStdString());

  addShapeSpecificSampleOptions(absCorAlgo, sampleShape);

  // Can details
  if (m_uiForm.ckUseCan->isChecked()) {
    const auto canWsName =
        m_uiForm.dsContainer->getCurrentDataName().toStdString();
    MatrixWorkspace_sptr canWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(canWsName);

    // If not in wavelength then do conversion
    Mantid::Kernel::Unit_sptr canXUnit = canWs->getAxis(0)->unit();
    if (canXUnit->caption() != "Wavelength" && emode != "Efixed") {
      g_log.information("Container workspace not in wavelength, need to "
                        "convert to continue.");
      absCorProps["CanWorkspace"] = addConvertUnitsStep(canWs, "Wavelength");
    } else {
      absCorProps["CanWorkspace"] = canWsName;
    }

    absCorAlgo->setProperty("CanDensityType",
                            m_uiForm.cbCanDensity->currentText().toStdString());
    absCorAlgo->setProperty("CanDensity", m_uiForm.spCanDensity->value());

    const auto canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
    absCorAlgo->setProperty("CanChemicalFormula",
                            canChemicalFormula.toStdString());

    addShapeSpecificCanOptions(absCorAlgo, sampleShape);
  }

  // Generate workspace names
  auto nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  const auto outputWsName =
      sampleWsName.left(nameCutIndex) + "_" + sampleShape + "_PP_Corrections";

  absCorAlgo->setProperty("OutputWorkspace", outputWsName.toStdString());

  // Add corrections algorithm to queue
  m_batchAlgoRunner->addAlgorithm(absCorAlgo, absCorProps);

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
  const auto sampleWsName = m_uiForm.dsSample->getCurrentDataName();
  const auto sampleWsNameStr = sampleWsName.toStdString();
  bool sampleExists =
      AnalysisDataService::Instance().doesExist(sampleWsNameStr);

  if (sampleExists &&
      !AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          sampleWsNameStr)) {
    uiv.addErrorMessage(
        "Invalid sample workspace. Ensure a MatrixWorkspace is provided.");
  }

  // Validate chemical formula
  if (uiv.checkFieldIsNotEmpty("Sample Chemical Formula",
                               m_uiForm.leSampleChemicalFormula,
                               m_uiForm.valSampleChemicalFormula))
    uiv.checkFieldIsValid("Sample Chemical Formula",
                          m_uiForm.leSampleChemicalFormula,
                          m_uiForm.valSampleChemicalFormula);
  const auto sampleChem =
      m_uiForm.leSampleChemicalFormula->text().toStdString();
  try {
    Mantid::Kernel::Material::parseChemicalFormula(sampleChem);
  } catch (std::runtime_error &ex) {
    UNUSED_ARG(ex);
    uiv.addErrorMessage("Chemical Formula for Sample was not recognised.");
    uiv.setErrorLabel(m_uiForm.valSampleChemicalFormula, false);
  }

  if (m_uiForm.ckUseCan->isChecked()) {
    uiv.checkDataSelectorIsValid("Can", m_uiForm.dsContainer);

    // Validate chemical formula
    if (uiv.checkFieldIsNotEmpty("Can Chemical Formula",
                                 m_uiForm.leCanChemicalFormula,
                                 m_uiForm.valCanChemicalFormula))
      uiv.checkFieldIsValid("Can Chemical Formula",
                            m_uiForm.leCanChemicalFormula,
                            m_uiForm.valCanChemicalFormula);

    const auto containerChem =
        m_uiForm.leCanChemicalFormula->text().toStdString();
    try {
      Mantid::Kernel::Material::parseChemicalFormula(containerChem);
    } catch (std::runtime_error &ex) {
      UNUSED_ARG(ex);
      uiv.addErrorMessage("Chemical Formula for Container was not recognised.");
      uiv.setErrorLabel(m_uiForm.valCanChemicalFormula, false);
    }

    const auto containerWsName = m_uiForm.dsContainer->getCurrentDataName();
    const auto containerWsNameStr = containerWsName.toStdString();
    bool containerExists =
        AnalysisDataService::Instance().doesExist(containerWsNameStr);

    if (containerExists &&
        !AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            containerWsNameStr)) {
      uiv.addErrorMessage(
          "Invalid container workspace. Ensure a MatrixWorkspace is provided.");
    }

    // Ensure sample and container are the same kind of data
    const auto sampleType = sampleWsName.right(sampleWsName.length() -
                                               sampleWsName.lastIndexOf("_"));
    const auto containerType = containerWsName.right(
        containerWsName.length() - containerWsName.lastIndexOf("_"));

    g_log.debug() << "Sample type is: " << sampleType.toStdString() << '\n';
    g_log.debug() << "Can type is: " << containerType.toStdString() << '\n';

    if (containerType != sampleType)
      uiv.addErrorMessage(
          "Sample and can workspaces must contain the same type of data.");

    // Shape validation

    const auto shape = m_uiForm.cbSampleShape->currentIndex();
    if (shape == 1 && m_uiForm.ckUseCan->isChecked()) {
      auto sampleRadius = m_uiForm.spCylSampleOuterRadius->value();
      auto containerRadius = m_uiForm.spCylCanOuterRadius->value();
      if (containerRadius <= sampleRadius) {
        uiv.addErrorMessage(
            "Container radius must be bigger than sample radius");
      }
    }
    if (shape == 2) {
      auto sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
      auto sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
      if (sampleOuterRadius <= sampleInnerRadius) {
        uiv.addErrorMessage(
            "Sample outer radius must be bigger than sample inner radius");
      }
      if (m_uiForm.ckUseCan->isChecked()) {
        auto containerRadius = m_uiForm.spAnnCanOuterRadius->value();
        if (containerRadius <= sampleOuterRadius) {
          uiv.addErrorMessage(
              "Container outer radius must be bigger than sample outer radius");
        }
      }
    }
  }

  // Show error message if needed
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
  const auto sampleWsName =
      m_uiForm.dsSample->getCurrentDataName().toStdString();
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sampleWsName);
  WorkspaceGroup_sptr corrections =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          m_pythonExportWsName);
  for (size_t i = 0; i < corrections->size(); i++) {
    MatrixWorkspace_sptr factorWs =
        boost::dynamic_pointer_cast<MatrixWorkspace>(corrections->getItem(i));
    if (!factorWs || !sampleWs)
      continue;

    if (getEMode(sampleWs) == "Indirect") {
      API::BatchAlgorithmRunner::AlgorithmRuntimeProps convertSpecProps;
      IAlgorithm_sptr convertSpecAlgo =
          AlgorithmManager::Instance().create("ConvertSpectrumAxis");
      convertSpecAlgo->initialize();
      convertSpecAlgo->setProperty("InputWorkspace", factorWs);
      convertSpecAlgo->setProperty("OutputWorkspace", factorWs->getName());
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

  // Enable post processing plot and save
  m_uiForm.cbPlotOutput->setEnabled(true);
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

void CalculatePaalmanPings::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsContainer->readSettings(settings.group());
}

/**
 * Slot that tries to populate correction details from
 * instrument parameters on sample workspace selection
 * @param wsName Sample workspace name
 */
void CalculatePaalmanPings::fillCorrectionDetails(const QString &wsName) {
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());

  if (!ws) {
    displayInvalidWorkspaceTypeError(wsName.toStdString(), g_log);
    return;
  }

  try {
    m_uiForm.doubleEfixed->setValue(getEFixed(ws));
  } catch (std::runtime_error &) {
    // do nothing if there is no efixed
  }

  auto emode = QString::fromStdString(getEMode(ws));
  int index = m_uiForm.cbEmode->findText(emode);
  if (index != -1) {
    m_uiForm.cbEmode->setCurrentIndex(index);
  }

  auto inst = ws->getInstrument();
  if (inst) {
    if (inst->hasParameter("AbsorptionCorrectionNumberWavelength")) {
      m_uiForm.spNwave->setValue(
          inst->getIntParameter("AbsorptionCorrectionNumberWavelength")[0]);
    }
    if (inst->hasParameter("AbsorptionCorrectionInterpolate")) {
      bool interpolate =
          inst->getBoolParameter("AbsorptionCorrectionInterpolate")[0];
      m_uiForm.cbInterpolate->setChecked(interpolate);
    }
  }
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
    return;
  }

  auto instrument = ws->getInstrument();

  if (!instrument) {
    g_log.warning() << "Failed to find instrument parameters in the workspace "
                    << wsName.toStdString() << '\n';
    return;
  }

  const auto beamWidth =
      getInstrumentParameter(instrument, "Workflow.beam-width");

  if (beamWidth) {
    m_uiForm.spCylBeamWidth->setValue(beamWidth.get());
    m_uiForm.spAnnBeamWidth->setValue(beamWidth.get());
  }

  const auto beamHeight =
      getInstrumentParameter(instrument, "Workflow.beam-height");

  if (beamHeight) {
    m_uiForm.spCylBeamHeight->setValue(beamHeight.get());
    m_uiForm.spAnnBeamHeight->setValue(beamHeight.get());
  }
}

/**
 * Attempt to extract an instrument double parameter from a specified
 * instrument.
 *
 * @param instrument    The instrument to extract the parameter from.
 * @param parameterName The name of the parameter to extract.
 *
 * @return              The extracted parameter if it is found, else
 *                      boost::none.
 */
boost::optional<double> CalculatePaalmanPings::getInstrumentParameter(
    Mantid::Geometry::Instrument_const_sptr instrument,
    const std::string &parameterName) {

  if (instrument->hasParameter(parameterName)) {
    const auto parameterValue = QString::fromStdString(
        instrument->getStringParameter(parameterName)[0]);
    return parameterValue.toDouble();
  }
  return boost::none;
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
    const auto sampleThickness = m_uiForm.spFlatSampleThickness->value();
    alg->setProperty("SampleThickness", sampleThickness);

    const auto sampleAngle = m_uiForm.spFlatSampleAngle->value();
    alg->setProperty("SampleAngle", sampleAngle);
  } else if (shape == "Cylinder") {
    alg->setProperty("SampleInnerRadius", 0.0);

    const auto sampleOuterRadius = m_uiForm.spCylSampleOuterRadius->value();
    alg->setProperty("SampleOuterRadius", sampleOuterRadius);

    const auto beamWidth = m_uiForm.spCylBeamWidth->value();
    alg->setProperty("BeamWidth", beamWidth);

    const auto beamHeight = m_uiForm.spCylBeamHeight->value();
    alg->setProperty("BeamHeight", beamHeight);

    const auto stepSize = m_uiForm.spCylStepSize->value();
    alg->setProperty("StepSize", stepSize);
  } else if (shape == "Annulus") {
    const auto sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
    alg->setProperty("SampleInnerRadius", sampleInnerRadius);

    const auto sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
    alg->setProperty("SampleOuterRadius", sampleOuterRadius);

    const auto beamWidth = m_uiForm.spAnnBeamWidth->value();
    alg->setProperty("BeamWidth", beamWidth);

    const auto beamHeight = m_uiForm.spAnnBeamHeight->value();
    alg->setProperty("BeamHeight", beamHeight);

    const auto stepSize = m_uiForm.spAnnStepSize->value();
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
    const auto canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
    alg->setProperty("CanFrontThickness", canFrontThickness);

    const auto canBackThickness = m_uiForm.spFlatCanBackThickness->value();
    alg->setProperty("CanBackThickness", canBackThickness);
  } else if (shape == "Cylinder") {
    const auto canOuterRadius = m_uiForm.spCylCanOuterRadius->value();
    alg->setProperty("CanOuterRadius", canOuterRadius);
  } else if (shape == "Annulus") {
    const auto canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
    alg->setProperty("CanOuterRadius", canOuterRadius);
  }
}

/**
 * Handles saving of workspace
 */
void CalculatePaalmanPings::saveClicked() {

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles mantid plotting of workspace
 */
void CalculatePaalmanPings::plotClicked() {

  QString plotType = m_uiForm.cbPlotOutput->currentText();

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true)) {

    if (plotType == "Both" || plotType == "Wavelength")
      plotSpectrum(QString::fromStdString(m_pythonExportWsName));

    if (plotType == "Both" || plotType == "Angle")
      plotTimeBin(QString::fromStdString(m_pythonExportWsName));
  }
}

/**
 * Handle changing of the sample density unit
 */
void CalculatePaalmanPings::changeSampleDensityUnit(int index) {

  if (index == 0) {
    m_uiForm.spSampleDensity->setSuffix(" g/cm3");
  } else {
    m_uiForm.spSampleDensity->setSuffix(" /A3");
  }
}

/**
 * Handle changing of the can density unit
 */
void CalculatePaalmanPings::changeCanDensityUnit(int index) {

  if (index == 0) {
    m_uiForm.spCanDensity->setSuffix(" g/cm3");
  } else {
    m_uiForm.spCanDensity->setSuffix(" /A3");
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
