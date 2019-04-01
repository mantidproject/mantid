// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "AbsorptionCorrections.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QRegExpValidator>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
Mantid::Kernel::Logger g_log("AbsorptionCorrections");

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

WorkspaceGroup_sptr getADSWorkspaceGroup(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      workspaceName);
}

template <typename T>
void addWorkspaceToADS(std::string const &workspaceName, T const &workspace) {
  AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
}

MatrixWorkspace_sptr convertUnits(MatrixWorkspace_sptr workspace,
                                  std::string const &target) {
  auto convertAlg = AlgorithmManager::Instance().create("ConvertUnits");
  convertAlg->initialize();
  convertAlg->setChild(true);
  convertAlg->setProperty("InputWorkspace", workspace);
  convertAlg->setProperty("OutputWorkspace", "__converted");
  convertAlg->setProperty(
      "EMode", Mantid::Kernel::DeltaEMode::asString(workspace->getEMode()));
  convertAlg->setProperty("EFixed",
                          workspace->getEFixed(workspace->getDetector(0)));
  convertAlg->setProperty("Target", target);
  convertAlg->execute();
  return convertAlg->getProperty("OutputWorkspace");
}

WorkspaceGroup_sptr
groupWorkspaces(std::vector<std::string> const &workspaceNames) {
  auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setChild(true);
  groupAlg->setProperty("InputWorkspaces", workspaceNames);
  groupAlg->setProperty("OutputWorkspace", "__grouped");
  groupAlg->execute();
  return groupAlg->getProperty("OutputWorkspace");
}

WorkspaceGroup_sptr convertUnits(WorkspaceGroup_sptr workspaceGroup,
                                 std::string const &target) {
  std::vector<std::string> convertedNames;
  convertedNames.reserve(workspaceGroup->size());

  for (auto const &workspace : *workspaceGroup) {
    auto const name = "__" + workspace->getName() + "_" + target;
    auto const wavelengthWorkspace = convertUnits(
        boost::dynamic_pointer_cast<MatrixWorkspace>(workspace), target);
    addWorkspaceToADS(name, wavelengthWorkspace);
    convertedNames.emplace_back(name);
  }
  return groupWorkspaces(convertedNames);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
AbsorptionCorrections::AbsorptionCorrections(QWidget *parent)
    : m_sampleDensities(std::make_shared<Densities>()),
      m_canDensities(std::make_shared<Densities>()), CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  QRegExp regex(R"([A-Za-z0-9\-\(\)]*)");
  QValidator *formulaValidator = new QRegExpValidator(regex, this);
  m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
  m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);

  // Change of input
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(getParameterDefaults(const QString &)));
  // Handle algorithm completion
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  // Handle running, plotting and saving
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  // Handle density units
  connect(m_uiForm.cbSampleDensity,
          SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(setSampleDensityUnit(QString const &)));
  connect(m_uiForm.cbCanDensity, SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(setCanDensityUnit(QString const &)));
  connect(m_uiForm.cbSampleDensity,
          SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(setSampleDensityValue(QString const &)));
  connect(m_uiForm.cbCanDensity, SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(setCanDensityValue(QString const &)));

  connect(m_uiForm.cbSampleMaterialMethod, SIGNAL(currentIndexChanged(int)),
          this, SLOT(changeSampleMaterialOptions(int)));
  connect(m_uiForm.cbCanMaterialMethod, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeCanMaterialOptions(int)));
  connect(m_uiForm.spSampleDensity, SIGNAL(valueChanged(double)), this,
          SLOT(setSampleDensity(double)));
  connect(m_uiForm.spCanDensity, SIGNAL(valueChanged(double)), this,
          SLOT(setCanDensity(double)));

  connect(m_uiForm.leSampleChemicalFormula, SIGNAL(editingFinished()), this,
          SLOT(doValidation()));
  connect(m_uiForm.leCanChemicalFormula, SIGNAL(editingFinished()), this,
          SLOT(doValidation()));
  connect(m_uiForm.ckUseCan, SIGNAL(stateChanged(int)), this,
          SLOT(doValidation()));
}

AbsorptionCorrections::~AbsorptionCorrections() {
  if (doesExistInADS("__mc_corrections_wavelength"))
    AnalysisDataService::Instance().remove("__mc_corrections_wavelength");
}

MatrixWorkspace_sptr AbsorptionCorrections::sampleWorkspace() const {
  auto const name = m_uiForm.dsSampleInput->getCurrentDataName().toStdString();
  return doesExistInADS(name) ? getADSMatrixWorkspace(name) : nullptr;
}

void AbsorptionCorrections::setup() { doValidation(); }

void AbsorptionCorrections::run() {
  setRunIsRunning(true);

  // Get correct corrections algorithm
  QString const sampleShape = m_uiForm.cbShape->currentText().replace(" ", "");

  IAlgorithm_sptr monteCarloAbsCor =
      AlgorithmManager::Instance().create("CalculateMonteCarloAbsorption");
  monteCarloAbsCor->initialize();

  monteCarloAbsCor->setProperty("Shape", sampleShape.toStdString());

  addShapeSpecificSampleOptions(monteCarloAbsCor, sampleShape);

  // Sample details
  QString const sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  monteCarloAbsCor->setProperty("SampleWorkspace", sampleWsName.toStdString());

  monteCarloAbsCor->setProperty(
      "SampleDensityType",
      m_uiForm.cbSampleDensity->currentText().toStdString());
  monteCarloAbsCor->setProperty("SampleDensity",
                                m_uiForm.spSampleDensity->value());

  if (m_uiForm.cbSampleMaterialMethod->currentText() == "Chemical Formula") {
    auto const sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
    monteCarloAbsCor->setProperty("SampleChemicalFormula",
                                  sampleChemicalFormula.toStdString());
  } else {
    monteCarloAbsCor->setProperty("SampleCoherentXSection",
                                  m_uiForm.spSampleCoherentXSection->value());
    monteCarloAbsCor->setProperty("SampleIncoherentXSection",
                                  m_uiForm.spSampleIncoherentXSection->value());
    monteCarloAbsCor->setProperty(
        "SampleAttenuationXSection",
        m_uiForm.spSampleAttenuationXSection->value());
  }

  // General details
  monteCarloAbsCor->setProperty("BeamHeight", m_uiForm.spBeamHeight->value());
  monteCarloAbsCor->setProperty("BeamWidth", m_uiForm.spBeamWidth->value());
  long const wave = static_cast<long>(m_uiForm.spNumberWavelengths->value());
  monteCarloAbsCor->setProperty("NumberOfWavelengthPoints", wave);
  long const events = static_cast<long>(m_uiForm.spNumberEvents->value());
  monteCarloAbsCor->setProperty("EventsPerPoint", events);
  auto const interpolation =
      m_uiForm.cbInterpolation->currentText().toStdString();
  monteCarloAbsCor->setProperty("Interpolation", interpolation);
  long const maxAttempts =
      static_cast<long>(m_uiForm.spMaxScatterPtAttempts->value());
  monteCarloAbsCor->setProperty("MaxScatterPtAttempts", maxAttempts);

  // Can details
  bool const useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {
    std::string const canWsName =
        m_uiForm.dsCanInput->getCurrentDataName().toStdString();
    monteCarloAbsCor->setProperty("ContainerWorkspace", canWsName);

    monteCarloAbsCor->setProperty(
        "ContainerDensityType",
        m_uiForm.cbCanDensity->currentText().toStdString());
    monteCarloAbsCor->setProperty("ContainerDensity",
                                  m_uiForm.spCanDensity->value());

    if (m_uiForm.cbCanMaterialMethod->currentText() == "Chemical Formula") {
      auto const canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
      monteCarloAbsCor->setProperty("ContainerChemicalFormula",
                                    canChemicalFormula.toStdString());
    } else {
      monteCarloAbsCor->setProperty("ContainerCoherentXSection",
                                    m_uiForm.spCanCoherentXSection->value());
      monteCarloAbsCor->setProperty("ContainerIncoherentXSection",
                                    m_uiForm.spCanIncoherentXSection->value());
      monteCarloAbsCor->setProperty("ContainerAttenuationXSection",
                                    m_uiForm.spCanAttenuationXSection->value());
    }

    addShapeSpecificCanOptions(monteCarloAbsCor, sampleShape);
  }

  // Generate workspace names
  int nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  auto const outputWsName =
      sampleWsName.left(nameCutIndex) + "_" + sampleShape + "_MC_Corrections";

  monteCarloAbsCor->setProperty("CorrectionsWorkspace",
                                outputWsName.toStdString());

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
void AbsorptionCorrections::addShapeSpecificSampleOptions(IAlgorithm_sptr alg,
                                                          QString shape) {

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
void AbsorptionCorrections::addShapeSpecificCanOptions(IAlgorithm_sptr alg,
                                                       QString const &shape) {
  if (shape == "FlatPlate") {
    double const canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
    alg->setProperty("ContainerFrontThickness", canFrontThickness);

    double const canBackThickness = m_uiForm.spFlatCanBackThickness->value();
    alg->setProperty("ContainerBackThickness", canBackThickness);
  } else if (shape == "Cylinder") {
    double const canInnerRadius = m_uiForm.spCylSampleRadius->value();
    alg->setProperty("ContainerInnerRadius", canInnerRadius);

    double const canOuterRadius = m_uiForm.spCylCanOuterRadius->value();
    alg->setProperty("ContainerOuterRadius", canOuterRadius);

  } else if (shape == "Annulus") {
    double const canInnerRadius = m_uiForm.spAnnCanInnerRadius->value();
    alg->setProperty("ContainerInnerRadius", canInnerRadius);

    double const canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
    alg->setProperty("ContainerOuterRadius", canOuterRadius);
  }
}

bool AbsorptionCorrections::validate() {
  UserInputValidator uiv = doValidation();

  // Give error for failed validation
  if (!uiv.isAllInputValid()) {
    auto const error = uiv.generateErrorMessage();
    showMessageBox(error);
  }
  return uiv.isAllInputValid();
}

UserInputValidator AbsorptionCorrections::doValidation() {
  UserInputValidator uiv;
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

  if (!sampleWorkspace())
    uiv.addErrorMessage(
        "Invalid sample workspace. Ensure a MatrixWorkspace is provided.");

  if (m_uiForm.cbSampleMaterialMethod->currentText() == "Chemical Formula") {
    if (uiv.checkFieldIsNotEmpty("Sample Chemical Formula",
                                 m_uiForm.leSampleChemicalFormula,
                                 m_uiForm.valSampleChemicalFormula))
      uiv.checkFieldIsValid("Sample Chemical Formula",
                            m_uiForm.leSampleChemicalFormula,
                            m_uiForm.valSampleChemicalFormula);
    auto const sampleChem =
        m_uiForm.leSampleChemicalFormula->text().toStdString();
    try {
      Mantid::Kernel::Material::parseChemicalFormula(sampleChem);
    } catch (std::runtime_error &ex) {
      UNUSED_ARG(ex);
      uiv.addErrorMessage("Chemical Formula for Sample was not recognised.");
      uiv.setErrorLabel(m_uiForm.valSampleChemicalFormula, false);
    }
  }

  bool useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {

    uiv.checkDataSelectorIsValid("Container", m_uiForm.dsCanInput);

    auto const containerWsName =
        m_uiForm.dsCanInput->getCurrentDataName().toStdString();
    if (doesExistInADS(containerWsName) &&
        !getADSMatrixWorkspace(containerWsName))
      uiv.addErrorMessage(
          "Invalid container workspace. Ensure a MatrixWorkspace is provided.");

    if (m_uiForm.cbCanMaterialMethod->currentText() == "Chemical Formula") {
      auto const containerChem =
          m_uiForm.leCanChemicalFormula->text().toStdString();
      if (uiv.checkFieldIsNotEmpty("Container Chemical Formula",
                                   m_uiForm.leCanChemicalFormula,
                                   m_uiForm.valCanChemicalFormula)) {
        uiv.checkFieldIsValid("Container Chemical Formula",
                              m_uiForm.leCanChemicalFormula,
                              m_uiForm.valCanChemicalFormula);
      }

      try {
        Mantid::Kernel::Material::parseChemicalFormula(containerChem);
      } catch (std::runtime_error &ex) {
        UNUSED_ARG(ex);
        uiv.addErrorMessage(
            "Chemical Formula for Container was not recognised.");
        uiv.setErrorLabel(m_uiForm.valCanChemicalFormula, false);
      }
    }
  } else
    uiv.setErrorLabel(m_uiForm.valCanChemicalFormula, true);

  return uiv;
}

void AbsorptionCorrections::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
  m_uiForm.dsCanInput->readSettings(settings.group());
}

void AbsorptionCorrections::processWavelengthWorkspace() {
  auto const correctionsWs = getADSWorkspaceGroup(m_pythonExportWsName);
  if (correctionsWs) {
    auto const wavelengthWorkspace = convertUnits(correctionsWs, "Wavelength");
    addWorkspaceToADS("__mc_corrections_wavelength", wavelengthWorkspace);
  }
}

/**
 * Handle completion of the absorption correction algorithm.
 *
 * @param error True if algorithm has failed.
 */
void AbsorptionCorrections::algorithmComplete(bool error) {
  setRunIsRunning(false);
  if (!error)
    processWavelengthWorkspace();
  else {
    setPlotResultEnabled(false);
    setSaveResultEnabled(false);
    emit showMessageBox(
        "Could not run absorption corrections.\nSee Results Log for details.");
  }
}

void AbsorptionCorrections::getParameterDefaults(QString const &dataName) {
  auto const sampleWs = getADSMatrixWorkspace(dataName.toStdString());
  if (sampleWs)
    getParameterDefaults(sampleWs->getInstrument());
  else
    displayInvalidWorkspaceTypeError(dataName.toStdString(), g_log);
}

void AbsorptionCorrections::getParameterDefaults(
    Instrument_const_sptr instrument) {
  setBeamWidthValue(instrument, "Workflow.beam-width");
  setBeamHeightValue(instrument, "Workflow.beam-height");
  setWavelengthsValue(instrument, "Workflow.absorption-wavelengths");
  setEventsValue(instrument, "Workflow.absorption-events");
  setInterpolationValue(instrument, "Workflow.absorption-interpolation");
  setMaxAttemptsValue(instrument, "Workflow.absorption-attempts");
}

void AbsorptionCorrections::setBeamWidthValue(
    Instrument_const_sptr instrument,
    std::string const &beamWidthParamName) const {
  if (instrument->hasParameter(beamWidthParamName)) {
    auto const beamWidth = QString::fromStdString(
        instrument->getStringParameter(beamWidthParamName)[0]);
    auto const beamWidthValue = beamWidth.toDouble();
    m_uiForm.spBeamWidth->setValue(beamWidthValue);
  }
}

void AbsorptionCorrections::setBeamHeightValue(
    Instrument_const_sptr instrument,
    std::string const &beamHeightParamName) const {
  if (instrument->hasParameter(beamHeightParamName)) {
    auto const beamHeight = QString::fromStdString(
        instrument->getStringParameter(beamHeightParamName)[0]);
    auto const beamHeightValue = beamHeight.toDouble();
    m_uiForm.spBeamHeight->setValue(beamHeightValue);
  }
}

void AbsorptionCorrections::setWavelengthsValue(
    Instrument_const_sptr instrument,
    std::string const &wavelengthsParamName) const {
  if (instrument->hasParameter(wavelengthsParamName)) {
    auto const wavelengths = QString::fromStdString(
        instrument->getStringParameter(wavelengthsParamName)[0]);
    auto const wavelengthsValue = wavelengths.toInt();
    m_uiForm.spNumberWavelengths->setValue(wavelengthsValue);
  }
}

void AbsorptionCorrections::setEventsValue(
    Instrument_const_sptr instrument,
    std::string const &eventsParamName) const {
  if (instrument->hasParameter(eventsParamName)) {
    auto const events = QString::fromStdString(
        instrument->getStringParameter(eventsParamName)[0]);
    auto const eventsValue = events.toInt();
    m_uiForm.spNumberEvents->setValue(eventsValue);
  }
}

void AbsorptionCorrections::setInterpolationValue(
    Instrument_const_sptr instrument,
    std::string const &interpolationParamName) const {
  if (instrument->hasParameter(interpolationParamName)) {
    auto const interpolation = QString::fromStdString(
        instrument->getStringParameter(interpolationParamName)[0]);
    auto const interpolationValue = interpolation.toStdString();
    m_uiForm.cbInterpolation->setCurrentIndex(
        interpolationValue == "CSpline" ? 1 : 0);
  }
}

void AbsorptionCorrections::setMaxAttemptsValue(
    Instrument_const_sptr instrument,
    std::string const &maxAttemptsParamName) const {
  if (instrument->hasParameter(maxAttemptsParamName)) {
    auto const maxScatterAttempts = QString::fromStdString(
        instrument->getStringParameter(maxAttemptsParamName)[0]);
    auto const maxScatterAttemptsValue = maxScatterAttempts.toInt();
    m_uiForm.spMaxScatterPtAttempts->setValue(maxScatterAttemptsValue);
  }
}

void AbsorptionCorrections::addSaveWorkspace(std::string const &workspaceName) {
  if (checkADSForPlotSaveWorkspace(workspaceName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(workspaceName));
}

void AbsorptionCorrections::saveClicked() {
  auto const factorsWs = m_absCorAlgo->getPropertyValue("CorrectionsWorkspace");
  addSaveWorkspace(m_pythonExportWsName);
  addSaveWorkspace(factorsWs);
  m_batchAlgoRunner->executeBatchAsync();
}

void AbsorptionCorrections::plotClicked() {
  setPlotResultIsPlotting(true);
  auto const plotType = m_uiForm.cbPlotOutput->currentText();

  if (checkADSForPlotSaveWorkspace("__mc_corrections_wavelength", false)) {
    if (plotType == "Both" || plotType == "Wavelength")
      plotSpectrum(QString::fromStdString("__mc_corrections_wavelength"));

    if (plotType == "Both" || plotType == "Angle")
      plotTimeBin(QString::fromStdString("__mc_corrections_wavelength"));
  }
  setPlotResultIsPlotting(false);
}

void AbsorptionCorrections::runClicked() { runTab(); }

void AbsorptionCorrections::setSampleDensityOptions(QString const &method) {
  setComboBoxOptions(m_uiForm.cbSampleDensity, getDensityOptions(method));
}

void AbsorptionCorrections::setCanDensityOptions(QString const &method) {
  setComboBoxOptions(m_uiForm.cbCanDensity, getDensityOptions(method));
}

void AbsorptionCorrections::setComboBoxOptions(
    QComboBox *combobox, std::vector<std::string> const &options) {
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
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spSampleDensity);
  m_uiForm.spSampleDensity->setValue(getSampleDensityValue(text));
}

void AbsorptionCorrections::setCanDensityValue(QString const &text) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spCanDensity);
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

std::vector<std::string>
AbsorptionCorrections::getDensityOptions(QString const &method) const {
  std::vector<std::string> densityOptions;
  if (method == "Chemical Formula")
    densityOptions.emplace_back("Mass Density");
  densityOptions.emplace_back("Atom Number Density");
  densityOptions.emplace_back("Formula Number Density");
  return densityOptions;
}

QString AbsorptionCorrections::getDensityUnit(QString const &type) const {
  auto const unit = type == "Mass Density"
                        ? m_sampleDensities->getMassDensityUnit()
                        : m_sampleDensities->getNumberDensityUnit();
  return QString::fromStdString(unit);
}

double AbsorptionCorrections::getSampleDensityValue(QString const &type) const {
  return type == "Mass Density" ? m_sampleDensities->getMassDensity()
                                : m_sampleDensities->getNumberDensity();
}

double AbsorptionCorrections::getCanDensityValue(QString const &type) const {
  return type == "Mass Density" ? m_canDensities->getMassDensity()
                                : m_canDensities->getNumberDensity();
}

void AbsorptionCorrections::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void AbsorptionCorrections::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlotOutput->setEnabled(enabled);
}

void AbsorptionCorrections::setSaveResultEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void AbsorptionCorrections::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void AbsorptionCorrections::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void AbsorptionCorrections::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
