// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "CalculatePaalmanPings.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceSelector.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"

#include <QLineEdit>
#include <QSignalBlocker>
#include <QValidator>

using namespace Mantid::API;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
Mantid::Kernel::Logger g_log("CalculatePaalmanPings");

std::string extractFirstOf(std::string const &str, std::string const &delimiter) {
  auto const cutIndex = str.find(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}

} // namespace

namespace MantidQt::CustomInterfaces {
CalculatePaalmanPings::CalculatePaalmanPings(QWidget *parent)
    : CorrectionsTab(parent), m_sampleDensities(std::make_shared<Densities>()),
      m_canDensities(std::make_shared<Densities>()) {
  m_uiForm.setupUi(parent);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  std::map<std::string, std::string> actions;
  actions["Plot Spectra"] = "Plot Wavelength";
  actions["Plot Bins"] = "Plot Angle";
  setOutputPlotOptionsPresenter(m_uiForm.ipoPlotOptions, PlotWidget::SpectraBin, "", actions);

  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(getBeamWidthFromWorkspace(const QString &)));

  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this, SLOT(fillCorrectionDetails(const QString &)));

  QRegExp regex(R"([A-Za-z0-9\-\(\)]*)");
  QValidator *formulaValidator = new QRegExpValidator(regex, this);
  m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
  m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);
  // Connect slots for run, plot and save
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  // Connect slots for toggling the mass/number density unit
  connect(m_uiForm.cbSampleDensity, SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(setSampleDensityUnit(QString const &)));
  connect(m_uiForm.cbCanDensity, SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(setCanDensityUnit(QString const &)));
  connect(m_uiForm.cbSampleDensity, SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(setSampleDensityValue(QString const &)));
  connect(m_uiForm.cbCanDensity, SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(setCanDensityValue(QString const &)));

  connect(m_uiForm.cbSampleMaterialMethod, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeSampleMaterialOptions(int)));
  connect(m_uiForm.cbCanMaterialMethod, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCanMaterialOptions(int)));
  connect(m_uiForm.spSampleDensity, SIGNAL(valueChanged(double)), this, SLOT(setSampleDensity(double)));
  connect(m_uiForm.spCanDensity, SIGNAL(valueChanged(double)), this, SLOT(setCanDensity(double)));

  // Allows empty workspace selector when initially selected
  m_uiForm.dsSample->isOptional(true);
  m_uiForm.dsContainer->isOptional(true);

  UserInputValidator uiv;
  if (uiv.checkFieldIsNotEmpty("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula)) {
    uiv.checkFieldIsValid("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula);
  }
}

void CalculatePaalmanPings::handleValidation(IUserInputValidator *validator) const {
  validator->checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  const auto sampleWsName = m_uiForm.dsSample->getCurrentDataName();
  const auto sampleWsNameStr = sampleWsName.toStdString();
  bool sampleExists = AnalysisDataService::Instance().doesExist(sampleWsNameStr);

  if (sampleExists && !AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sampleWsNameStr)) {
    validator->addErrorMessage("Invalid sample workspace. Ensure a MatrixWorkspace is provided.");
  }

  // Validate chemical formula
  if (m_uiForm.cbSampleMaterialMethod->currentText() == "Chemical Formula") {
    if (validator->checkFieldIsNotEmpty("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula,
                                        m_uiForm.valSampleChemicalFormula))
      validator->checkFieldIsValid("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula,
                                   m_uiForm.valSampleChemicalFormula);

    const auto sampleChem = m_uiForm.leSampleChemicalFormula->text().toStdString();
    try {
      Mantid::Kernel::Material::parseChemicalFormula(sampleChem);
    } catch (std::runtime_error &ex) {
      UNUSED_ARG(ex);
      validator->addErrorMessage("Chemical Formula for Sample was not recognised.");
      validator->setErrorLabel(m_uiForm.valSampleChemicalFormula, false);
    }
  }

  if (m_uiForm.ckUseCan->isChecked()) {
    validator->checkDataSelectorIsValid("Can", m_uiForm.dsContainer);

    if (m_uiForm.cbCanMaterialMethod->currentText() == "Chemical Formula") {
      // Validate chemical formula
      if (validator->checkFieldIsNotEmpty("Can Chemical Formula", m_uiForm.leCanChemicalFormula,
                                          m_uiForm.valCanChemicalFormula))
        validator->checkFieldIsValid("Can Chemical Formula", m_uiForm.leCanChemicalFormula,
                                     m_uiForm.valCanChemicalFormula);

      const auto containerChem = m_uiForm.leCanChemicalFormula->text().toStdString();
      try {
        Mantid::Kernel::Material::parseChemicalFormula(containerChem);
      } catch (std::runtime_error &ex) {
        UNUSED_ARG(ex);
        validator->addErrorMessage("Chemical Formula for Container was not recognised.");
        validator->setErrorLabel(m_uiForm.valCanChemicalFormula, false);
      }
    }

    const auto containerWsName = m_uiForm.dsContainer->getCurrentDataName();
    const auto containerWsNameStr = containerWsName.toStdString();
    bool containerExists = AnalysisDataService::Instance().doesExist(containerWsNameStr);

    if (containerExists && !AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(containerWsNameStr)) {
      validator->addErrorMessage("Invalid container workspace. Ensure a MatrixWorkspace is provided.");
    }

    // Ensure sample and container are the same kind of data
    const auto sampleType = sampleWsName.right(sampleWsName.length() - sampleWsName.lastIndexOf("_"));
    const auto containerType = containerWsName.right(containerWsName.length() - containerWsName.lastIndexOf("_"));

    g_log.debug() << "Sample type is: " << sampleType.toStdString() << '\n';
    g_log.debug() << "Can type is: " << containerType.toStdString() << '\n';

    if (containerType != sampleType)
      validator->addErrorMessage("Sample and can workspaces must contain the same type of data.");

    // Shape validation

    const auto shape = m_uiForm.cbSampleShape->currentIndex();
    if (shape == 1 && m_uiForm.ckUseCan->isChecked()) {
      auto sampleRadius = m_uiForm.spCylSampleOuterRadius->value();
      auto containerRadius = m_uiForm.spCylCanOuterRadius->value();
      if (containerRadius <= sampleRadius) {
        validator->addErrorMessage("Container radius must be bigger than sample radius");
      }
    }
    if (shape == 2) {
      auto sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
      auto sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
      if (sampleOuterRadius <= sampleInnerRadius) {
        validator->addErrorMessage("Sample outer radius must be bigger than sample inner radius");
      }
      if (m_uiForm.ckUseCan->isChecked()) {
        auto containerRadius = m_uiForm.spAnnCanOuterRadius->value();
        if (containerRadius <= sampleOuterRadius) {
          validator->addErrorMessage("Container outer radius must be bigger than sample outer radius");
        }
      }
    }
  }
}

void CalculatePaalmanPings::handleRun() {
  clearOutputPlotOptionsWorkspaces();

  // Get correct corrections algorithm
  auto sampleShape = m_uiForm.cbSampleShape->currentText();
  auto algorithmName = sampleShape.replace(" ", "") + "PaalmanPingsCorrection";
  algorithmName = algorithmName.replace("Annulus", "Cylinder"); // Use the cylinder algorithm for annulus

  auto absCorProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  auto absCorAlgo = AlgorithmManager::Instance().create(algorithmName.toStdString());
  absCorAlgo->initialize();

  // Sample details
  auto sampleWsName = m_uiForm.dsSample->getCurrentDataName();
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sampleWsName.toStdString());

  const auto emode = m_uiForm.cbEmode->currentText().toStdString();
  absCorAlgo->setProperty("EMode", emode);

  const auto efixed = m_uiForm.doubleEfixed->value();
  absCorAlgo->setProperty("EFixed", efixed);

  const auto numwave = m_uiForm.spNwave->value();
  absCorAlgo->setProperty("NumberWavelengths", numwave);

  const bool inter = m_uiForm.cbInterpolate->isChecked();
  absCorAlgo->setProperty("Interpolate", inter);

  // If not in wavelength then do conversion
  const auto sampleXUnit = sampleWs->getAxis(0)->unit();
  if (sampleXUnit->caption() != "Wavelength" && emode != "Efixed") {
    g_log.information("Sample workspace not in wavelength, need to convert to continue.");

    auto const convertedSampleWorkspace = addConvertUnitsStep(sampleWs, "Wavelength", "UNIT", emode, efixed);
    if (convertedSampleWorkspace)
      absCorProps->setPropertyValue("SampleWorkspace", convertedSampleWorkspace.value());
    else {
      m_runPresenter->setRunEnabled(true);
      return;
    }

  } else {
    absCorProps->setPropertyValue("SampleWorkspace", sampleWsName.toStdString());
  }

  auto const sampleDensityType = m_uiForm.cbSampleDensity->currentText().toStdString();
  absCorAlgo->setProperty("SampleDensityType", getDensityType(sampleDensityType));
  if (sampleDensityType != "Mass Density")
    absCorAlgo->setProperty("SampleNumberDensityUnit", getNumberDensityUnit(sampleDensityType));

  absCorAlgo->setProperty("SampleDensity", m_uiForm.spSampleDensity->value());

  if (m_uiForm.cbSampleMaterialMethod->currentText() == "Chemical Formula") {
    absCorAlgo->setProperty("SampleChemicalFormula", m_uiForm.leSampleChemicalFormula->text().toStdString());
  } else {
    absCorAlgo->setProperty("SampleCoherentXSection", m_uiForm.spSampleCoherentXSection->value());
    absCorAlgo->setProperty("SampleIncoherentXSection", m_uiForm.spSampleIncoherentXSection->value());
    absCorAlgo->setProperty("SampleAttenuationXSection", m_uiForm.spSampleAttenuationXSection->value());
  }

  addShapeSpecificSampleOptions(absCorAlgo, sampleShape);

  // Can details
  if (m_uiForm.ckUseCan->isChecked()) {
    const auto canWsName = m_uiForm.dsContainer->getCurrentDataName().toStdString();
    MatrixWorkspace_sptr canWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(canWsName);

    // If not in wavelength then do conversion
    Mantid::Kernel::Unit_sptr canXUnit = canWs->getAxis(0)->unit();
    if (canXUnit->caption() != "Wavelength" && emode != "Efixed") {
      g_log.information("Container workspace not in wavelength, need to "
                        "convert to continue.");

      auto const convertedWorkspace = addConvertUnitsStep(canWs, "Wavelength", "UNIT", emode);
      if (convertedWorkspace)
        absCorProps->setPropertyValue("CanWorkspace", convertedWorkspace.value());
      else {
        m_runPresenter->setRunEnabled(true);
        return;
      }

    } else {
      absCorProps->setPropertyValue("CanWorkspace", canWsName);
    }

    auto const canDensityType = m_uiForm.cbCanDensity->currentText().toStdString();
    absCorAlgo->setProperty("CanDensityType", getDensityType(canDensityType));
    if (canDensityType != "Mass Density")
      absCorAlgo->setProperty("CanNumberDensityUnit", getNumberDensityUnit(canDensityType));

    absCorAlgo->setProperty("CanDensity", m_uiForm.spCanDensity->value());

    if (m_uiForm.cbCanMaterialMethod->currentText() == "Chemical Formula") {
      absCorAlgo->setProperty("CanChemicalFormula", m_uiForm.leCanChemicalFormula->text().toStdString());
    } else {
      absCorAlgo->setProperty("CanCoherentXSection", m_uiForm.spCanCoherentXSection->value());
      absCorAlgo->setProperty("CanIncoherentXSection", m_uiForm.spCanIncoherentXSection->value());
      absCorAlgo->setProperty("CanAttenuationXSection", m_uiForm.spCanAttenuationXSection->value());
    }

    addShapeSpecificCanOptions(absCorAlgo, sampleShape);
  }

  // Generate workspace names
  auto nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  const auto outputWsName = sampleWsName.left(nameCutIndex) + "_" + sampleShape + "_PP_Corrections";

  absCorAlgo->setProperty("OutputWorkspace", outputWsName.toStdString());

  // Add corrections algorithm to queue
  m_batchAlgoRunner->addAlgorithm(absCorAlgo, std::move(absCorProps));

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(absCorComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = outputWsName.toStdString();
}

/**
 * Handles completion of the correction algorithm.
 *
 * @param error True if the algorithm failed
 */
void CalculatePaalmanPings::absCorComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(absCorComplete(bool)));
  m_runPresenter->setRunEnabled(true);

  if (!error) {
    // Convert the spectrum axis of correction factors to Q
    const auto sampleWsName = m_uiForm.dsSample->getCurrentDataName().toStdString();
    MatrixWorkspace_sptr sampleWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sampleWsName);
    WorkspaceGroup_sptr corrections = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(m_pythonExportWsName);
    for (size_t i = 0; i < corrections->size(); i++) {
      MatrixWorkspace_sptr factorWs = std::dynamic_pointer_cast<MatrixWorkspace>(corrections->getItem(i));
      if (!factorWs || !sampleWs)
        continue;

      if (getEMode(sampleWs) == "Indirect") {
        auto convertSpecAlgo = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
        convertSpecAlgo->initialize();
        convertSpecAlgo->setProperty("InputWorkspace", factorWs);
        convertSpecAlgo->setProperty("OutputWorkspace", factorWs->getName());
        convertSpecAlgo->setProperty("Target", "ElasticQ");
        convertSpecAlgo->setProperty("EMode", "Indirect");

        if (auto const eFixed = getEFixed(factorWs)) {
          convertSpecAlgo->setProperty("EFixed", *eFixed);
        }

        m_batchAlgoRunner->addAlgorithm(convertSpecAlgo);
      }
    }

    // Run algorithm queue
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(postProcessComplete(bool)));
    m_batchAlgoRunner->executeBatchAsync();
  } else {
    setSaveResultEnabled(false);
    emit showMessageBox("Absorption correction calculation failed.\nSee "
                        "Results Log for more details.");
  }
}

/**
 * Handles completion of the post processing algorithms.
 *
 * @param error True of the algorithm failed
 */
void CalculatePaalmanPings::postProcessComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(postProcessComplete(bool)));
  m_runPresenter->setRunEnabled(true);

  if (!error) {
    auto const group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(m_pythonExportWsName);
    for (auto const &workspace : *group) {
      auto correctionsWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
      correctionsWorkspace->setYUnit("");
      correctionsWorkspace->setYUnitLabel("Attenuation Factor");
    }
    setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
  } else {
    setSaveResultEnabled(false);
    emit showMessageBox("Correction factor post processing failed.\nSee "
                        "Results Log for more details.");
  }
}

void CalculatePaalmanPings::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsContainer->readSettings(settings.group());
}

void CalculatePaalmanPings::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("CalculatePaalmanPings");
  m_uiForm.dsSample->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsSample->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
  m_uiForm.dsContainer->setFBSuffixes(filter ? getContainerFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsContainer->setWSSuffixes(filter ? getContainerWSSuffixes(tabName) : noSuffixes);
}

/**
 * Slot that tries to populate correction details from
 * instrument parameters on sample workspace selection
 * @param wsName Sample workspace name
 */
void CalculatePaalmanPings::fillCorrectionDetails(const QString &wsName) {
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());

  if (!ws) {
    displayInvalidWorkspaceTypeError(wsName.toStdString(), g_log);
    return;
  }

  if (auto const eFixed = getEFixed(ws)) {
    m_uiForm.doubleEfixed->setValue(*eFixed);
  }

  auto emode = QString::fromStdString(getEMode(ws));
  int index = m_uiForm.cbEmode->findText(emode);
  if (index != -1) {
    m_uiForm.cbEmode->setCurrentIndex(index);
  }

  auto inst = ws->getInstrument();
  if (inst) {
    if (inst->hasParameter("AbsorptionCorrectionNumberWavelength")) {
      m_uiForm.spNwave->setValue(inst->getIntParameter("AbsorptionCorrectionNumberWavelength")[0]);
    }
    if (inst->hasParameter("AbsorptionCorrectionInterpolate")) {
      bool interpolate = inst->getBoolParameter("AbsorptionCorrectionInterpolate")[0];
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
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());

  if (!ws) {
    return;
  }

  auto instrument = ws->getInstrument();

  if (!instrument) {
    g_log.warning() << "Failed to find instrument parameters in the workspace " << wsName.toStdString() << '\n';
    return;
  }

  const auto beamWidth = getInstrumentParameter(instrument, "Workflow.beam-width");

  if (beamWidth) {
    m_uiForm.spCylBeamWidth->setValue(beamWidth.value());
    m_uiForm.spAnnBeamWidth->setValue(beamWidth.value());
  }

  const auto beamHeight = getInstrumentParameter(instrument, "Workflow.beam-height");

  if (beamHeight) {
    m_uiForm.spCylBeamHeight->setValue(beamHeight.value());
    m_uiForm.spAnnBeamHeight->setValue(beamHeight.value());
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
 *                      std::nullopt.
 */
std::optional<double>
CalculatePaalmanPings::getInstrumentParameter(const Mantid::Geometry::Instrument_const_sptr &instrument,
                                              const std::string &parameterName) {

  if (instrument->hasParameter(parameterName)) {
    const auto parameterValue = QString::fromStdString(instrument->getStringParameter(parameterName)[0]);
    return parameterValue.toDouble();
  }
  return std::nullopt;
}

/**
 * Sets algorithm properties specific to the sample for a given shape.
 *
 * @param alg Algorithm to set properties of
 * @param shape Sample shape
 */
void CalculatePaalmanPings::addShapeSpecificSampleOptions(const IAlgorithm_sptr &alg, const QString &shape) {
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
void CalculatePaalmanPings::addShapeSpecificCanOptions(const IAlgorithm_sptr &alg, const QString &shape) {
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

void CalculatePaalmanPings::saveClicked() {

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

void CalculatePaalmanPings::setSampleDensityOptions(QString const &method) {
  setComboBoxOptions(m_uiForm.cbSampleDensity, getDensityOptions(method));
}

void CalculatePaalmanPings::setCanDensityOptions(QString const &method) {
  setComboBoxOptions(m_uiForm.cbCanDensity, getDensityOptions(method));
}

void CalculatePaalmanPings::setComboBoxOptions(QComboBox *combobox, std::vector<std::string> const &options) {
  combobox->clear();
  for (auto const &option : options)
    combobox->addItem(QString::fromStdString(option));
}

void CalculatePaalmanPings::setSampleDensityUnit(QString const &text) {
  m_uiForm.spSampleDensity->setSuffix(getDensityUnit(text));
}

void CalculatePaalmanPings::setCanDensityUnit(QString const &text) {
  m_uiForm.spCanDensity->setSuffix(getDensityUnit(text));
}

void CalculatePaalmanPings::setSampleDensityValue(QString const &text) {
  QSignalBlocker blocker(m_uiForm.spSampleDensity);
  m_uiForm.spSampleDensity->setValue(getSampleDensityValue(text));
}

void CalculatePaalmanPings::setCanDensityValue(QString const &text) {
  QSignalBlocker blocker(m_uiForm.spCanDensity);
  m_uiForm.spCanDensity->setValue(getCanDensityValue(text));
}

void CalculatePaalmanPings::changeSampleMaterialOptions(int index) {
  setSampleDensityOptions(m_uiForm.cbSampleMaterialMethod->currentText());
  m_uiForm.swSampleMaterialDetails->setCurrentIndex(index);
}

void CalculatePaalmanPings::changeCanMaterialOptions(int index) {
  setCanDensityOptions(m_uiForm.cbCanMaterialMethod->currentText());
  m_uiForm.swCanMaterialDetails->setCurrentIndex(index);
}

void CalculatePaalmanPings::setSampleDensity(double value) {
  if (m_uiForm.cbSampleDensity->currentText() == "Mass Density")
    m_sampleDensities->setMassDensity(value);
  else
    m_sampleDensities->setNumberDensity(value);
}

void CalculatePaalmanPings::setCanDensity(double value) {
  if (m_uiForm.cbCanDensity->currentText() == "Mass Density")
    m_canDensities->setMassDensity(value);
  else
    m_canDensities->setNumberDensity(value);
}

std::vector<std::string> CalculatePaalmanPings::getDensityOptions(QString const &method) const {
  std::vector<std::string> densityOptions;
  if (method == "Chemical Formula")
    densityOptions.emplace_back("Mass Density");
  densityOptions.emplace_back("Atom Number Density");
  densityOptions.emplace_back("Formula Number Density");
  return densityOptions;
}

std::string CalculatePaalmanPings::getDensityType(std::string const &type) const {
  return type == "Mass Density" ? type : "Number Density";
}

std::string CalculatePaalmanPings::getNumberDensityUnit(std::string const &type) const {
  return extractFirstOf(type, " ") == "Formula" ? "Formula Units" : "Atoms";
}

QString CalculatePaalmanPings::getDensityUnit(QString const &type) const {
  auto const unit =
      type == "Mass Density" ? m_sampleDensities->getMassDensityUnit() : m_sampleDensities->getNumberDensityUnit();
  return QString::fromStdString(unit);
}

double CalculatePaalmanPings::getSampleDensityValue(QString const &type) const {
  return type == "Mass Density" ? m_sampleDensities->getMassDensity() : m_sampleDensities->getNumberDensity();
}

double CalculatePaalmanPings::getCanDensityValue(QString const &type) const {
  return type == "Mass Density" ? m_canDensities->getMassDensity() : m_canDensities->getNumberDensity();
}

void CalculatePaalmanPings::setSaveResultEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

} // namespace MantidQt::CustomInterfaces
