#include "AbsorptionCorrections.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"

#include <QRegExpValidator>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("AbsorptionCorrections");

MatrixWorkspace_sptr convertUnits(MatrixWorkspace_sptr workspace,
                                  const std::string &target) {
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
groupWorkspaces(const std::vector<std::string> &workspaceNames) {
  auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setChild(true);
  groupAlg->setProperty("InputWorkspaces", workspaceNames);
  groupAlg->setProperty("OutputWorkspace", "__grouped");
  groupAlg->execute();
  return groupAlg->getProperty("OutputWorkspace");
}

WorkspaceGroup_sptr convertUnits(WorkspaceGroup_sptr workspaceGroup,
                                 const std::string &target) {
  std::vector<std::string> convertedNames;
  convertedNames.reserve(workspaceGroup->size());

  for (const auto &workspace : *workspaceGroup) {
    const auto name = "__" + workspace->getName() + "_" + target;
    const auto wavelengthWorkspace = convertUnits(
        boost::dynamic_pointer_cast<MatrixWorkspace>(workspace), target);
    AnalysisDataService::Instance().addOrReplace(name, wavelengthWorkspace);
    convertedNames.emplace_back(name);
  }
  return groupWorkspaces(convertedNames);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
AbsorptionCorrections::AbsorptionCorrections(QWidget *parent)
    : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  QRegExp regex(R"([A-Za-z0-9\-\(\)]*)");
  QValidator *formulaValidator = new QRegExpValidator(regex, this);
  m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
  m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);

  // Change of input
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(getBeamDefaults(const QString &)));

  // Handle algorithm completion
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  // Handle plotting and saving
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));

  // Handle density units
  connect(m_uiForm.cbSampleDensity, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeSampleDensityUnit(int)));
  connect(m_uiForm.cbCanDensity, SIGNAL(currentIndexChanged(int)), this,
          SLOT(changeCanDensityUnit(int)));

  connect(m_uiForm.leSampleChemicalFormula, SIGNAL(editingFinished()), this,
          SLOT(doValidation()));
  connect(m_uiForm.leCanChemicalFormula, SIGNAL(editingFinished()), this,
          SLOT(doValidation()));
  connect(m_uiForm.ckUseCan, SIGNAL(stateChanged(int)), this,
          SLOT(doValidation()));
}

AbsorptionCorrections::~AbsorptionCorrections() {
  if (AnalysisDataService::Instance().doesExist("__mc_corrections_wavelength"))
    AnalysisDataService::Instance().remove("__mc_corrections_wavelength");
}

MatrixWorkspace_sptr AbsorptionCorrections::sampleWorkspace() const {
  const auto sampleWSName =
      m_uiForm.dsSampleInput->getCurrentDataName().toStdString();

  if (AnalysisDataService::Instance().doesExist(sampleWSName))
    return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        sampleWSName);
  return nullptr;
}

void AbsorptionCorrections::setup() { doValidation(); }

void AbsorptionCorrections::run() {
  // Get correct corrections algorithm
  QString sampleShape = m_uiForm.cbShape->currentText().replace(" ", "");

  IAlgorithm_sptr monteCarloAbsCor =
      AlgorithmManager::Instance().create("CalculateMonteCarloAbsorption");
  monteCarloAbsCor->initialize();

  monteCarloAbsCor->setProperty("Shape", sampleShape.toStdString());

  addShapeSpecificSampleOptions(monteCarloAbsCor, sampleShape);

  // Sample details
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  monteCarloAbsCor->setProperty("SampleWorkspace", sampleWsName.toStdString());

  monteCarloAbsCor->setProperty(
      "SampleDensityType",
      m_uiForm.cbSampleDensity->currentText().toStdString());
  monteCarloAbsCor->setProperty("SampleDensity",
                                m_uiForm.spSampleDensity->value());

  QString sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
  monteCarloAbsCor->setProperty("SampleChemicalFormula",
                                sampleChemicalFormula.toStdString());

  // General details
  monteCarloAbsCor->setProperty("BeamHeight", m_uiForm.spBeamHeight->value());
  monteCarloAbsCor->setProperty("BeamWidth", m_uiForm.spBeamWidth->value());
  long wave = static_cast<long>(m_uiForm.spNumberWavelengths->value());
  monteCarloAbsCor->setProperty("NumberOfWavelengthPoints", wave);
  long events = static_cast<long>(m_uiForm.spNumberEvents->value());
  monteCarloAbsCor->setProperty("EventsPerPoint", events);
  auto const interpolation =
      m_uiForm.cbInterpolation->currentText().toStdString();
  monteCarloAbsCor->setProperty("Interpolation", interpolation);
  long maxAttempts =
      static_cast<long>(m_uiForm.spMaxScatterPtAttempts->value());
  monteCarloAbsCor->setProperty("MaxScatterPtAttempts", maxAttempts);

  // Can details
  bool useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {
    std::string canWsName =
        m_uiForm.dsCanInput->getCurrentDataName().toStdString();
    monteCarloAbsCor->setProperty("ContainerWorkspace", canWsName);

    monteCarloAbsCor->setProperty(
        "ContainerDensityType",
        m_uiForm.cbCanDensity->currentText().toStdString());
    monteCarloAbsCor->setProperty("ContainerDensity",
                                  m_uiForm.spCanDensity->value());

    const auto canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
    monteCarloAbsCor->setProperty("ContainerChemicalFormula",
                                  canChemicalFormula.toStdString());

    addShapeSpecificCanOptions(monteCarloAbsCor, sampleShape);
  }

  // Generate workspace names
  int nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  const auto outputWsName =
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
    double sampleHeight = m_uiForm.spFlatSampleHeight->value();
    alg->setProperty("Height", sampleHeight);

    double sampleWidth = m_uiForm.spFlatSampleWidth->value();
    alg->setProperty("SampleWidth", sampleWidth);

    double sampleThickness = m_uiForm.spFlatSampleThickness->value();
    alg->setProperty("SampleThickness", sampleThickness);

    double sampleAngle = m_uiForm.spFlatSampleAngle->value();
    alg->setProperty("SampleAngle", sampleAngle);

  } else if (shape == "Annulus") {
    double sampleHeight = m_uiForm.spAnnSampleHeight->value();
    alg->setProperty("Height", sampleHeight);

    double sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
    alg->setProperty("SampleInnerRadius", sampleInnerRadius);

    double sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
    alg->setProperty("SampleOuterRadius", sampleOuterRadius);

  } else if (shape == "Cylinder") {
    double sampleRadius = m_uiForm.spCylSampleRadius->value();
    alg->setProperty("SampleRadius", sampleRadius);

    double sampleHeight = m_uiForm.spCylSampleHeight->value();
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
                                                       QString shape) {
  if (shape == "FlatPlate") {
    double canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
    alg->setProperty("ContainerFrontThickness", canFrontThickness);

    double canBackThickness = m_uiForm.spFlatCanBackThickness->value();
    alg->setProperty("ContainerBackThickness", canBackThickness);
  } else if (shape == "Cylinder") {
    double canInnerRadius = m_uiForm.spCylSampleRadius->value();
    alg->setProperty("ContainerInnerRadius", canInnerRadius);

    double canOuterRadius = m_uiForm.spCylCanOuterRadius->value();
    alg->setProperty("ContainerOuterRadius", canOuterRadius);

  } else if (shape == "Annulus") {
    double canInnerRadius = m_uiForm.spAnnCanInnerRadius->value();
    alg->setProperty("ContainerInnerRadius", canInnerRadius);

    double canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
    alg->setProperty("ContainerOuterRadius", canOuterRadius);
  }
}

bool AbsorptionCorrections::validate() {
  UserInputValidator uiv = doValidation();

  // Give error for failed validation
  if (!uiv.isAllInputValid()) {
    QString error = uiv.generateErrorMessage();
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

  bool useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {
    const auto containerChem =
        m_uiForm.leCanChemicalFormula->text().toStdString();
    uiv.checkDataSelectorIsValid("Container", m_uiForm.dsCanInput);

    const auto containerWsName =
        m_uiForm.dsCanInput->getCurrentDataName().toStdString();
    bool containerExists =
        AnalysisDataService::Instance().doesExist(containerWsName);
    if (containerExists &&
        !AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            containerWsName)) {
      uiv.addErrorMessage(
          "Invalid container workspace. Ensure a MatrixWorkspace is provided.");
    }

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
      uiv.addErrorMessage("Chemical Formula for Container was not recognised.");
      uiv.setErrorLabel(m_uiForm.valCanChemicalFormula, false);
    }
  } else
    uiv.setErrorLabel(m_uiForm.valCanChemicalFormula, true);

  return uiv;
}

void AbsorptionCorrections::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
  m_uiForm.dsCanInput->readSettings(settings.group());
}

/**
 * Handle completion of the absorption correction algorithm.
 *
 * @param error True if algorithm has failed.
 */
void AbsorptionCorrections::algorithmComplete(bool error) {
  if (error) {
    emit showMessageBox(
        "Could not run absorption corrections.\nSee Results Log for details.");
    return;
  }

  auto correctionsWorkspace =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          m_pythonExportWsName);

  if (correctionsWorkspace) {
    auto wavelengthWorkspace = convertUnits(correctionsWorkspace, "Wavelength");
    AnalysisDataService::Instance().addOrReplace("__mc_corrections_wavelength",
                                                 wavelengthWorkspace);

    // Enable plot and save
    m_uiForm.pbPlot->setEnabled(true);
    m_uiForm.cbPlotOutput->setEnabled(true);
    m_uiForm.pbSave->setEnabled(true);
  }
}

void AbsorptionCorrections::getBeamDefaults(const QString &dataName) {
  auto sampleWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      dataName.toStdString());

  if (!sampleWs) {
    displayInvalidWorkspaceTypeError(dataName.toStdString(), g_log);
    return;
  }

  auto instrument = sampleWs->getInstrument();
  const std::string beamWidthParamName = "Workflow.beam-width";
  if (instrument->hasParameter(beamWidthParamName)) {
    const auto beamWidth = QString::fromStdString(
        instrument->getStringParameter(beamWidthParamName)[0]);
    const auto beamWidthValue = beamWidth.toDouble();

    m_uiForm.spBeamWidth->setValue(beamWidthValue);
  }
  const std::string beamHeightParamName = "Workflow.beam-height";
  if (instrument->hasParameter(beamHeightParamName)) {
    const auto beamHeight = QString::fromStdString(
        instrument->getStringParameter(beamHeightParamName)[0]);
    const auto beamHeightValue = beamHeight.toDouble();

    m_uiForm.spBeamHeight->setValue(beamHeightValue);
  }
}

/**
 * Handle saving of workspace
 */
void AbsorptionCorrections::saveClicked() {

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));

  std::string factorsWs =
      m_absCorAlgo->getPropertyValue("CorrectionsWorkspace");
  if (checkADSForPlotSaveWorkspace(factorsWs, false))
    addSaveWorkspaceToQueue(QString::fromStdString(factorsWs));

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle mantid plotting
 */
void AbsorptionCorrections::plotClicked() {
  const auto plotType = m_uiForm.cbPlotOutput->currentText();

  if (checkADSForPlotSaveWorkspace("__mc_corrections_wavelength", false)) {
    if (plotType == "Both" || plotType == "Wavelength")
      plotSpectrum(QString::fromStdString("__mc_corrections_wavelength"));

    if (plotType == "Both" || plotType == "Angle")
      plotTimeBin(QString::fromStdString("__mc_corrections_wavelength"));
  }
}

/**
 * Handle changing of the sample density unit
 */
void AbsorptionCorrections::changeSampleDensityUnit(int index) {

  if (index == 0) {
    m_uiForm.spSampleDensity->setSuffix(" g/cm3");
  } else {
    m_uiForm.spSampleDensity->setSuffix(" 1/A3");
  }
}

/**
 * Handle changing of the container density unit
 */
void AbsorptionCorrections::changeCanDensityUnit(int index) {

  if (index == 0) {
    m_uiForm.spCanDensity->setSuffix(" g/cm3");
  } else {
    m_uiForm.spCanDensity->setSuffix(" 1/A3");
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
