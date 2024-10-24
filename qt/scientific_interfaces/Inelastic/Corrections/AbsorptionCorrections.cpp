// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "AbsorptionCorrections.h"
#include "MantidAPI/Axis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include <QRegExpValidator>
#include <QSignalBlocker>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::DeltaEMode;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

/**
 * Determines whether an input has a value of zero
 * @param Input value
 * @return True if zero, False if not
 */
static bool isValueZero(double value) { return value == 0; }

namespace {
Mantid::Kernel::Logger g_log("AbsorptionCorrections");

template <typename T> void addWorkspaceToADS(std::string const &workspaceName, T const &workspace) {
  AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
}

std::string extractFirstOf(std::string const &str, std::string const &delimiter) {
  auto const cutIndex = str.find(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}

void setYAxisLabels(const WorkspaceGroup_sptr &group, std::string const &unit, std::string const &axisLabel) {
  for (auto const &workspace : *group) {
    auto matrixWs = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    matrixWs->setYUnit(unit);
    matrixWs->setYUnitLabel(axisLabel);
  }
}

void convertSpectrumAxis(const MatrixWorkspace_sptr &workspace, double eFixed = 0.0) {
  auto convertAlg = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  convertAlg->initialize();
  convertAlg->setProperty("InputWorkspace", workspace);
  convertAlg->setProperty("OutputWorkspace", workspace->getName());
  convertAlg->setProperty("Target", "ElasticQ");
  convertAlg->setProperty("EMode", "Indirect");
  if (eFixed != 0.0)
    convertAlg->setProperty("EFixed", eFixed);
  convertAlg->execute();
}

MatrixWorkspace_sptr convertUnits(const MatrixWorkspace_sptr &workspace, std::string const &target) {
  auto convertAlg = AlgorithmManager::Instance().create("ConvertUnits");
  convertAlg->initialize();
  convertAlg->setChild(true);
  convertAlg->setProperty("InputWorkspace", workspace);
  convertAlg->setProperty("OutputWorkspace", "__converted");
  convertAlg->setProperty("EMode", DeltaEMode::asString(workspace->getEMode()));
  if (auto const eFixed = getEFixed(workspace)) {
    convertAlg->setProperty("EFixed", *eFixed);
  }
  convertAlg->setProperty("Target", target);
  convertAlg->execute();
  return convertAlg->getProperty("OutputWorkspace");
}

WorkspaceGroup_sptr groupWorkspaces(std::vector<std::string> const &workspaceNames) {
  auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setChild(true);
  groupAlg->setProperty("InputWorkspaces", workspaceNames);
  groupAlg->setProperty("OutputWorkspace", "__grouped");
  groupAlg->execute();
  return groupAlg->getProperty("OutputWorkspace");
}

WorkspaceGroup_sptr convertUnits(const WorkspaceGroup_sptr &workspaceGroup, std::string const &target) {
  std::vector<std::string> convertedNames;
  convertedNames.reserve(workspaceGroup->size());

  for (auto const &workspace : *workspaceGroup) {
    auto const name = workspace->getName();
    auto const wavelengthWorkspace = convertUnits(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), target);
    addWorkspaceToADS(name, wavelengthWorkspace);
    convertedNames.emplace_back(name);
  }
  return groupWorkspaces(convertedNames);
}
} // namespace

namespace MantidQt::CustomInterfaces {
AbsorptionCorrections::AbsorptionCorrections(QWidget *parent)
    : CorrectionsTab(parent), m_sampleDensities(std::make_shared<Densities>()),
      m_canDensities(std::make_shared<Densities>()) {
  m_uiForm.setupUi(parent);
  std::map<std::string, std::string> actions;
  actions["Plot Spectra"] = "Plot Wavelength";
  actions["Plot Bins"] = "Plot Angle";
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  setOutputPlotOptionsPresenter(m_uiForm.ipoPlotOptions, PlotWidget::SpectraBin, "", actions);

  QRegExp regex(R"([A-Za-z0-9\-\(\)]*)");
  QValidator *formulaValidator = new QRegExpValidator(regex, this);
  m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
  m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);

  // Change of input

  connect(m_uiForm.dsSampleInput, &DataSelector::dataReady, this,
          static_cast<void (AbsorptionCorrections::*)(const QString &)>(&AbsorptionCorrections::getParameterDefaults));
  connect(m_uiForm.cbShape, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &AbsorptionCorrections::handlePresetShapeChanges);
  // Handle algorithm completion
  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this,
          &AbsorptionCorrections::algorithmComplete);
  // Handle running, plotting and saving
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &AbsorptionCorrections::saveClicked);
  // Handle density units

  connect(m_uiForm.cbSampleDensity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int index) { this->setSampleDensityUnit(m_uiForm.cbSampleDensity->itemText(index)); });
  connect(m_uiForm.cbCanDensity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int index) { this->setCanDensityUnit(m_uiForm.cbCanDensity->itemText(index)); });
  connect(m_uiForm.cbSampleDensity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int index) { this->setSampleDensityValue(m_uiForm.cbSampleDensity->itemText(index)); });
  connect(m_uiForm.cbCanDensity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int index) { this->setCanDensityValue(m_uiForm.cbCanDensity->itemText(index)); });
  connect(m_uiForm.cbSampleMaterialMethod, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &AbsorptionCorrections::changeSampleMaterialOptions);
  connect(m_uiForm.cbCanMaterialMethod, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &AbsorptionCorrections::changeCanMaterialOptions);
  connect(m_uiForm.spSampleDensity, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &AbsorptionCorrections::setSampleDensity);
  connect(m_uiForm.spCanDensity, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &AbsorptionCorrections::setCanDensity);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsSampleInput->isOptional(true);
}

AbsorptionCorrections::~AbsorptionCorrections() = default;

MatrixWorkspace_sptr AbsorptionCorrections::sampleWorkspace() const {
  auto const name = m_uiForm.dsSampleInput->getCurrentDataName().toStdString();
  return doesExistInADS(name) ? getADSWorkspace(name) : nullptr;
}

void AbsorptionCorrections::handleValidation(IUserInputValidator *validator) const {
  validator->checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

  if (!sampleWorkspace())
    validator->addErrorMessage("Invalid sample workspace. Ensure a MatrixWorkspace is provided.");

  QString const sampleShape = m_uiForm.cbShape->currentText().replace(" ", "");
  const bool isPreset = sampleShape == "Preset";
  const bool isUseCan = m_uiForm.cbUseCan->isChecked();

  if (m_uiForm.cbSampleMaterialMethod->currentText() == "Chemical Formula") {
    if (!(m_uiForm.leSampleChemicalFormula->text().isEmpty() && isPreset)) {
      validator->checkFieldIsValid("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula,
                                   m_uiForm.valSampleChemicalFormula);
      auto const sampleChem = m_uiForm.leSampleChemicalFormula->text().toStdString();
      try {
        Mantid::Kernel::Material::parseChemicalFormula(sampleChem);
      } catch (std::runtime_error &ex) {
        UNUSED_ARG(ex);
        validator->addErrorMessage("Chemical Formula for Sample was not recognised.");
        validator->setErrorLabel(m_uiForm.valSampleChemicalFormula, false);
      }
    }
  }

  if (!isPreset) {
    validateSampleGeometryInputs(validator, sampleShape);

    if (isUseCan) {
      if (m_uiForm.cbCanMaterialMethod->currentText() == "Chemical Formula") {
        auto const containerChem = m_uiForm.leCanChemicalFormula->text().toStdString();
        if (validator->checkFieldIsNotEmpty("Container Chemical Formula", m_uiForm.leCanChemicalFormula,
                                            m_uiForm.valCanChemicalFormula)) {
          validator->checkFieldIsValid("Container Chemical Formula", m_uiForm.leCanChemicalFormula,
                                       m_uiForm.valCanChemicalFormula);
        }

        try {
          Mantid::Kernel::Material::parseChemicalFormula(containerChem);
        } catch (std::runtime_error &ex) {
          UNUSED_ARG(ex);
          validator->addErrorMessage("Chemical Formula for Container was not recognised.");
          validator->setErrorLabel(m_uiForm.valCanChemicalFormula, false);
        }
      }
      validateContainerGeometryInputs(validator, sampleShape);
    }
  }
}

void AbsorptionCorrections::handleRun() {
  setSaveResultEnabled(false);

  bool const isUseCan = m_uiForm.cbUseCan->isChecked();

  IAlgorithm_sptr monteCarloAbsCor = AlgorithmManager::Instance().create("PaalmanPingsMonteCarloAbsorption");
  monteCarloAbsCor->initialize();

  // Sample details
  QString const sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  monteCarloAbsCor->setProperty("InputWorkspace", sampleWsName.toStdString());

  // General details
  monteCarloAbsCor->setProperty("BeamHeight", m_uiForm.spBeamHeight->value());
  monteCarloAbsCor->setProperty("BeamWidth", m_uiForm.spBeamWidth->value());
  auto const events = m_uiForm.spNumberEvents->value();
  monteCarloAbsCor->setProperty("EventsPerPoint", events);
  auto const interpolation = m_uiForm.cbInterpolation->currentText().toStdString();
  monteCarloAbsCor->setProperty("Interpolation", interpolation);
  auto const maxAttempts = m_uiForm.spMaxScatterPtAttempts->value();
  monteCarloAbsCor->setProperty("MaxScatterPtAttempts", maxAttempts);

  bool const isSparseInstrument = m_uiForm.cbSparseInstrument->isChecked();
  if (isSparseInstrument) {
    monteCarloAbsCor->setProperty("SparseInstrument", isSparseInstrument);
    auto const numberOfDetectorRows = m_uiForm.spNumberDetectorRows->value();
    monteCarloAbsCor->setProperty("NumberOfDetectorRows", numberOfDetectorRows);
    auto const numberOfDetectorColumns = m_uiForm.spNumberDetectorColumns->value();
    monteCarloAbsCor->setProperty("NumberOfDetectorColumns", numberOfDetectorColumns);
  }

  QString const sampleShape = m_uiForm.cbShape->currentText().replace(" ", "");
  const bool isPreset = sampleShape == "Preset";
  monteCarloAbsCor->setProperty("Shape", sampleShape.toStdString());

  auto const sampleDensityType = m_uiForm.cbSampleDensity->currentText().toStdString();
  monteCarloAbsCor->setProperty("SampleDensityType", getDensityType(sampleDensityType));
  if (sampleDensityType != "Mass Density")
    monteCarloAbsCor->setProperty("SampleNumberDensityUnit", getNumberDensityUnit(sampleDensityType));

  monteCarloAbsCor->setProperty("SampleDensity", m_uiForm.spSampleDensity->value());

  if (m_uiForm.cbSampleMaterialMethod->currentText() == "Chemical Formula") {
    auto const sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
    monteCarloAbsCor->setProperty("SampleChemicalFormula", sampleChemicalFormula.toStdString());
  } else {
    monteCarloAbsCor->setProperty("SampleCoherentXSection", m_uiForm.spSampleCoherentXSection->value());
    monteCarloAbsCor->setProperty("SampleIncoherentXSection", m_uiForm.spSampleIncoherentXSection->value());
    monteCarloAbsCor->setProperty("SampleAttenuationXSection", m_uiForm.spSampleAttenuationXSection->value());
  }

  if (!isPreset) {

    addShapeSpecificSampleOptions(monteCarloAbsCor, sampleShape);

    if (isUseCan) {

      // Can details
      auto const containerDensityType = m_uiForm.cbCanDensity->currentText().toStdString();
      monteCarloAbsCor->setProperty("ContainerDensityType", getDensityType(containerDensityType));
      if (containerDensityType != "Mass Density")
        monteCarloAbsCor->setProperty("ContainerNumberDensityUnit", getNumberDensityUnit(containerDensityType));

      monteCarloAbsCor->setProperty("ContainerDensity", m_uiForm.spCanDensity->value());

      if (m_uiForm.cbCanMaterialMethod->currentText() == "Chemical Formula") {
        auto const canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
        monteCarloAbsCor->setProperty("ContainerChemicalFormula", canChemicalFormula.toStdString());
      } else {
        monteCarloAbsCor->setProperty("ContainerCoherentXSection", m_uiForm.spCanCoherentXSection->value());
        monteCarloAbsCor->setProperty("ContainerIncoherentXSection", m_uiForm.spCanIncoherentXSection->value());
        monteCarloAbsCor->setProperty("ContainerAttenuationXSection", m_uiForm.spCanAttenuationXSection->value());
      }

      addShapeSpecificCanOptions(monteCarloAbsCor, sampleShape);
    }
  }

  // Generate workspace names
  int nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  auto const outputWsName = sampleWsName.left(nameCutIndex) + "_" + sampleShape + "_MC_Corrections";

  monteCarloAbsCor->setProperty("CorrectionsWorkspace", outputWsName.toStdString());

  // Add correction algorithm to batch
  m_batchAlgoRunner->addAlgorithm(monteCarloAbsCor);

  m_absCorAlgo = monteCarloAbsCor;

  // Run algorithm batch
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = outputWsName.toStdString();
}

/**
 * Sets algorithm properties specific to the sample for a given shape.
 *
 * @param alg Algorithm to set properties of
 * @param shape Sample shape
 */
void AbsorptionCorrections::addShapeSpecificSampleOptions(const IAlgorithm_sptr &alg, const QString &shape) {

  if (shape == "FlatPlate") {
    double const sampleHeight = m_uiForm.spFlatSampleHeight->value();
    alg->setProperty("Height", sampleHeight);

    double const sampleWidth = m_uiForm.spFlatSampleWidth->value();
    alg->setProperty("SampleWidth", sampleWidth);

    double const sampleThickness = m_uiForm.spFlatSampleThickness->value();
    alg->setProperty("SampleThickness", sampleThickness);

    double const sampleAngle = m_uiForm.spFlatSampleAngle->value();
    alg->setProperty("SampleAngle", sampleAngle);

  } else if (shape == "Annulus") {
    double const sampleHeight = m_uiForm.spAnnSampleHeight->value();
    alg->setProperty("Height", sampleHeight);

    double const sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
    alg->setProperty("SampleInnerRadius", sampleInnerRadius);

    double const sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
    alg->setProperty("SampleOuterRadius", sampleOuterRadius);

  } else if (shape == "Cylinder") {
    double const sampleRadius = m_uiForm.spCylSampleRadius->value();
    alg->setProperty("SampleRadius", sampleRadius);

    double const sampleHeight = m_uiForm.spCylSampleHeight->value();
    alg->setProperty("Height", sampleHeight);
  }
}

/**
 * Sets algorithm properties specific to the can for a given shape.
 *
 * All options for Annulus are added in addShapeSpecificSampleOptions.
 *
 * @param alg Algorithm to set properties of
 * @param shape Sample shape
 */
void AbsorptionCorrections::addShapeSpecificCanOptions(const IAlgorithm_sptr &alg, QString const &shape) {
  if (shape == "FlatPlate") {
    double const canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
    alg->setProperty("ContainerFrontThickness", canFrontThickness);

    double const canBackThickness = m_uiForm.spFlatCanBackThickness->value();
    alg->setProperty("ContainerBackThickness", canBackThickness);
  } else if (shape == "Cylinder") {
    double const canOuterRadius = m_uiForm.spCylCanOuterRadius->value();
    alg->setProperty("ContainerRadius", canOuterRadius);

  } else if (shape == "Annulus") {
    double const canInnerRadius = m_uiForm.spAnnCanInnerRadius->value();
    alg->setProperty("ContainerInnerRadius", canInnerRadius);

    double const canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
    alg->setProperty("ContainerOuterRadius", canOuterRadius);
  }
}

/**
 * Validates algorithm properties specific to the sample for a given shape.
 *
 * @param uiv Address of user input validator to set error messages omn
 * @param shape Sample shape
 */
void AbsorptionCorrections::validateSampleGeometryInputs(IUserInputValidator *uiv, const QString &shape) const {
  bool hasZero = false;
  if (shape == "FlatPlate") {
    double const sampleHeight = m_uiForm.spFlatSampleHeight->value();
    hasZero = hasZero || isValueZero(sampleHeight);

    double const sampleWidth = m_uiForm.spFlatSampleWidth->value();
    hasZero = hasZero || isValueZero(sampleWidth);

    double const sampleThickness = m_uiForm.spFlatSampleThickness->value();
    hasZero = hasZero || isValueZero(sampleThickness);

  } else if (shape == "Annulus") {

    double const sampleHeight = m_uiForm.spAnnSampleHeight->value();
    hasZero = hasZero || isValueZero(sampleHeight);

    double const sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
    hasZero = hasZero || isValueZero(sampleInnerRadius);

    double const sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
    hasZero = hasZero || isValueZero(sampleOuterRadius);

    if (sampleInnerRadius >= sampleOuterRadius) {
      uiv->addErrorMessage("SampleOuterRadius must be greater than SampleInnerRadius.");
    }

  } else if (shape == "Cylinder") {
    double const sampleRadius = m_uiForm.spCylSampleRadius->value();
    hasZero = hasZero || isValueZero(sampleRadius);

    double const sampleHeight = m_uiForm.spCylSampleHeight->value();
    hasZero = hasZero || isValueZero(sampleHeight);
  }
  if (hasZero) {
    uiv->addErrorMessage("Sample Geometry inputs cannot be zero-valued.");
  }
}

/**
 * Validates algorithm properties specific to the container for a given shape.
 *
 * @param uiv Address of user input validator to set error messages omn
 * @param shape Container shape
 */
void AbsorptionCorrections::validateContainerGeometryInputs(IUserInputValidator *uiv, const QString &shape) const {
  bool hasZero = false;

  if (shape == "FlatPlate") {
    double const canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
    hasZero = hasZero || isValueZero(canFrontThickness);

    double const canBackThickness = m_uiForm.spFlatCanBackThickness->value();
    hasZero = hasZero || isValueZero(canBackThickness);
  } else if (shape == "Cylinder") {
    double const canOuterRadius = m_uiForm.spCylCanOuterRadius->value();
    hasZero = hasZero || isValueZero(canOuterRadius);

    double const sampleRadius = m_uiForm.spAnnSampleOuterRadius->value();
    if (canOuterRadius <= sampleRadius) {
      uiv->addErrorMessage("CanOuterRadius must be greater than SampleRadius.");
    }

  } else if (shape == "Annulus") {
    double const canInnerRadius = m_uiForm.spAnnCanInnerRadius->value();
    hasZero = hasZero || isValueZero(canInnerRadius);

    double const canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
    hasZero = hasZero || isValueZero(canOuterRadius);

    double const sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
    double const sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
    if (canInnerRadius >= sampleInnerRadius) {
      uiv->addErrorMessage("SampleInnerRadius must be greater than ContainerInnerRadius.");
    }
    if (canOuterRadius <= sampleOuterRadius) {
      uiv->addErrorMessage("ContainerOuterRadius must be greater than SampleOuterRadius.");
    }
  }
  if (hasZero) {
    uiv->addErrorMessage("Container Geometry inputs cannot be zero-valued.");
  }
}

void AbsorptionCorrections::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
}

void AbsorptionCorrections::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("CalculateMonteCarlo");
  m_uiForm.dsSampleInput->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsSampleInput->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

void AbsorptionCorrections::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsSampleInput->setLoadProperty("LoadHistory", doLoadHistory);
}

void AbsorptionCorrections::processWavelengthWorkspace() {
  auto correctionsWs = getADSWorkspace<WorkspaceGroup>(m_pythonExportWsName);
  if (correctionsWs) {
    correctionsWs = convertUnits(correctionsWs, "Wavelength");
    addWorkspaceToADS(m_pythonExportWsName, correctionsWs);
  }

  convertSpectrumAxes(correctionsWs);
}

void AbsorptionCorrections::convertSpectrumAxes(const WorkspaceGroup_sptr &correctionsWs) {
  auto const sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName().toStdString();
  convertSpectrumAxes(correctionsWs, getADSWorkspace(sampleWsName));
  setYAxisLabels(correctionsWs, "", "Attenuation Factor");
}

void AbsorptionCorrections::convertSpectrumAxes(const WorkspaceGroup_sptr &correctionsGroup,
                                                const MatrixWorkspace_sptr &sample) {
  for (auto const &workspace : *correctionsGroup) {
    auto const correction = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    convertSpectrumAxes(correction, sample);
  }
}

void AbsorptionCorrections::convertSpectrumAxes(const MatrixWorkspace_sptr &correction,
                                                const MatrixWorkspace_sptr &sample) {
  if (correction && sample && sample->getEMode() == DeltaEMode::Type::Indirect) {
    if (auto const eFixed = getEFixed(correction)) {
      convertSpectrumAxis(correction, *eFixed);
    } else {
      convertSpectrumAxis(correction);
    }
  }
}

/**
 * Handle completion of the absorption correction algorithm.
 * @param error True if algorithm has failed.
 */
void AbsorptionCorrections::algorithmComplete(bool error) {
  m_runPresenter->setRunEnabled(true);
  setSaveResultEnabled(!error);
  // The m_saveAlgRunning flag is queried here so the
  // processWavelengthWorkspace isn't executed at the end of the
  // saveAlg completing, as this will throw an exception.
  if (!error && !m_saveAlgRunning) {
    processWavelengthWorkspace();
    setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
  } else if (!error && m_saveAlgRunning) {
    m_saveAlgRunning = false;
  } else {
    m_saveAlgRunning = false;
    emit showMessageBox("Could not run absorption corrections.\nSee Results Log for details.");
  }
}

void AbsorptionCorrections::getParameterDefaults(QString const &dataName) {
  auto const sampleWs = getADSWorkspace(dataName.toStdString());
  if (sampleWs)
    getParameterDefaults(sampleWs->getInstrument());
  else
    displayInvalidWorkspaceTypeError(dataName.toStdString(), g_log);
}

void AbsorptionCorrections::getParameterDefaults(const Instrument_const_sptr &instrument) {
  setBeamWidthValue(instrument, "Workflow.beam-width");
  setBeamHeightValue(instrument, "Workflow.beam-height");
  setEventsValue(instrument, "Workflow.absorption-events");
  setInterpolationValue(instrument, "Workflow.absorption-interpolation");
  setMaxAttemptsValue(instrument, "Workflow.absorption-attempts");
}

void AbsorptionCorrections::setBeamWidthValue(const Instrument_const_sptr &instrument,
                                              std::string const &beamWidthParamName) const {
  if (instrument->hasParameter(beamWidthParamName)) {
    auto const beamWidth = QString::fromStdString(instrument->getStringParameter(beamWidthParamName)[0]);
    auto const beamWidthValue = beamWidth.toDouble();
    m_uiForm.spBeamWidth->setValue(beamWidthValue);
  }
}

void AbsorptionCorrections::setBeamHeightValue(const Instrument_const_sptr &instrument,
                                               std::string const &beamHeightParamName) const {
  if (instrument->hasParameter(beamHeightParamName)) {
    auto const beamHeight = QString::fromStdString(instrument->getStringParameter(beamHeightParamName)[0]);
    auto const beamHeightValue = beamHeight.toDouble();
    m_uiForm.spBeamHeight->setValue(beamHeightValue);
  }
}

void AbsorptionCorrections::setEventsValue(const Instrument_const_sptr &instrument,
                                           std::string const &eventsParamName) const {
  if (instrument->hasParameter(eventsParamName)) {
    auto const events = QString::fromStdString(instrument->getStringParameter(eventsParamName)[0]);
    auto const eventsValue = events.toInt();
    m_uiForm.spNumberEvents->setValue(eventsValue);
  }
}

void AbsorptionCorrections::setInterpolationValue(const Instrument_const_sptr &instrument,
                                                  std::string const &interpolationParamName) const {
  if (instrument->hasParameter(interpolationParamName)) {
    auto const interpolation = QString::fromStdString(instrument->getStringParameter(interpolationParamName)[0]);
    auto const interpolationValue = interpolation.toStdString();
    m_uiForm.cbInterpolation->setCurrentIndex(interpolationValue == "CSpline" ? 1 : 0);
  }
}

void AbsorptionCorrections::setMaxAttemptsValue(const Instrument_const_sptr &instrument,
                                                std::string const &maxAttemptsParamName) const {
  if (instrument->hasParameter(maxAttemptsParamName)) {
    auto const maxScatterAttempts = QString::fromStdString(instrument->getStringParameter(maxAttemptsParamName)[0]);
    auto const maxScatterAttemptsValue = maxScatterAttempts.toInt();
    m_uiForm.spMaxScatterPtAttempts->setValue(maxScatterAttemptsValue);
  }
}

void AbsorptionCorrections::addSaveWorkspace(std::string const &workspaceName) {
  if (checkADSForPlotSaveWorkspace(workspaceName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(workspaceName));
}

void AbsorptionCorrections::saveClicked() {
  m_saveAlgRunning = true;
  auto const factorsWs = m_absCorAlgo->getPropertyValue("CorrectionsWorkspace");
  addSaveWorkspace(m_pythonExportWsName);
  addSaveWorkspace(factorsWs);
  m_batchAlgoRunner->executeBatchAsync();
}

void AbsorptionCorrections::setSampleDensityOptions(QString const &method) {
  setComboBoxOptions(m_uiForm.cbSampleDensity, getDensityOptions(method));
}

void AbsorptionCorrections::setCanDensityOptions(QString const &method) {
  setComboBoxOptions(m_uiForm.cbCanDensity, getDensityOptions(method));
}

void AbsorptionCorrections::setComboBoxOptions(QComboBox *combobox, std::vector<std::string> const &options) {
  combobox->clear();
  for (auto const &option : options)
    combobox->addItem(QString::fromStdString(option));
}

void AbsorptionCorrections::setSampleDensityUnit(QString const &text) {
  m_uiForm.spSampleDensity->setSuffix(getDensityUnit(text));
}

void AbsorptionCorrections::setCanDensityUnit(QString const &text) {
  m_uiForm.spCanDensity->setSuffix(getDensityUnit(text));
}

void AbsorptionCorrections::setSampleDensityValue(QString const &text) {
  QSignalBlocker blocker(m_uiForm.spSampleDensity);
  m_uiForm.spSampleDensity->setValue(getSampleDensityValue(text));
}

void AbsorptionCorrections::setCanDensityValue(QString const &text) {
  QSignalBlocker blocker(m_uiForm.spCanDensity);
  m_uiForm.spCanDensity->setValue(getCanDensityValue(text));
}

void AbsorptionCorrections::changeSampleMaterialOptions(int index) {
  setSampleDensityOptions(m_uiForm.cbSampleMaterialMethod->currentText());
  m_uiForm.swSampleMaterialDetails->setCurrentIndex(index);
}

void AbsorptionCorrections::changeCanMaterialOptions(int index) {
  setCanDensityOptions(m_uiForm.cbCanMaterialMethod->currentText());
  m_uiForm.swCanMaterialDetails->setCurrentIndex(index);
}

void AbsorptionCorrections::setSampleDensity(double value) {
  if (m_uiForm.cbSampleDensity->currentText() == "Mass Density")
    m_sampleDensities->setMassDensity(value);
  else
    m_sampleDensities->setNumberDensity(value);
}

void AbsorptionCorrections::setCanDensity(double value) {
  if (m_uiForm.cbCanDensity->currentText() == "Mass Density")
    m_canDensities->setMassDensity(value);
  else
    m_canDensities->setNumberDensity(value);
}

void AbsorptionCorrections::handlePresetShapeChanges(int index) {
  if (index == 0) {
    m_uiForm.cbUseCan->setChecked(true);
    m_uiForm.cbUseCan->setEnabled(false);
    m_uiForm.gbContainerDetails->setEnabled(false);
  } else {
    m_uiForm.cbUseCan->setEnabled(true);
    m_uiForm.gbContainerDetails->setEnabled(m_uiForm.cbUseCan->isChecked());
  }
}

std::vector<std::string> AbsorptionCorrections::getDensityOptions(QString const &method) const {
  std::vector<std::string> densityOptions;
  if (method == "Chemical Formula")
    densityOptions.emplace_back("Mass Density");
  densityOptions.emplace_back("Atom Number Density");
  densityOptions.emplace_back("Formula Number Density");
  return densityOptions;
}

std::string AbsorptionCorrections::getDensityType(std::string const &type) const {
  return type == "Mass Density" ? type : "Number Density";
}

std::string AbsorptionCorrections::getNumberDensityUnit(std::string const &type) const {
  return extractFirstOf(type, " ") == "Formula" ? "Formula Units" : "Atoms";
}

QString AbsorptionCorrections::getDensityUnit(QString const &type) const {
  auto const unit =
      type == "Mass Density" ? m_sampleDensities->getMassDensityUnit() : m_sampleDensities->getNumberDensityUnit();
  return QString::fromStdString(unit);
}

double AbsorptionCorrections::getSampleDensityValue(QString const &type) const {
  return type == "Mass Density" ? m_sampleDensities->getMassDensity() : m_sampleDensities->getNumberDensity();
}

double AbsorptionCorrections::getCanDensityValue(QString const &type) const {
  return type == "Mass Density" ? m_canDensities->getMassDensity() : m_canDensities->getNumberDensity();
}

void AbsorptionCorrections::setSaveResultEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

} // namespace MantidQt::CustomInterfaces
