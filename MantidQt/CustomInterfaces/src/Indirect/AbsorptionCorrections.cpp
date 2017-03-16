#include "MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"

#include <QRegExpValidator>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("AbsorptionCorrections");
}

namespace MantidQt {
namespace CustomInterfaces {
AbsorptionCorrections::AbsorptionCorrections(QWidget *parent)
    : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  QRegExp regex("[A-Za-z0-9\\-\\(\\)]*");
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
}

void AbsorptionCorrections::setup() {}

void AbsorptionCorrections::run() {
  // Get correct corrections algorithm
  QString sampleShape = m_uiForm.cbShape->currentText().replace(" ", "");
  QString algorithmName = "Indirect" + sampleShape + "Absorption";

  IAlgorithm_sptr absCorAlgo =
      AlgorithmManager::Instance().create(algorithmName.toStdString());
  absCorAlgo->initialize();

  // Sample details
  QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
  absCorAlgo->setProperty("SampleWorkspace", sampleWsName.toStdString());

  absCorAlgo->setProperty(
      "SampleDensityType",
      m_uiForm.cbSampleDensity->currentText().toStdString());
  absCorAlgo->setProperty("SampleDensity", m_uiForm.spSampleDensity->value());

  QString sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
  absCorAlgo->setProperty("SampleChemicalFormula",
                          sampleChemicalFormula.toStdString());

  addShapeSpecificSampleOptions(absCorAlgo, sampleShape);

  // General details
  absCorAlgo->setProperty("BeamHeight", m_uiForm.spBeamHeight->value());
  absCorAlgo->setProperty("BeamWidth", m_uiForm.spBeamWidth->value());
  long wave = static_cast<long>(m_uiForm.spNumberWavelengths->value());
  absCorAlgo->setProperty("NumberWavelengths", wave);
  long events = static_cast<long>(m_uiForm.spNumberEvents->value());
  absCorAlgo->setProperty("Events", events);

  // Can details
  bool useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {
    std::string canWsName =
        m_uiForm.dsCanInput->getCurrentDataName().toStdString();
    std::string shiftedCanName = canWsName + "_shifted";
    IAlgorithm_sptr clone =
        AlgorithmManager::Instance().create("CloneWorkspace");
    clone->initialize();
    clone->setProperty("InputWorkspace", canWsName);
    clone->setProperty("OutputWorkspace", shiftedCanName);
    clone->execute();

    MatrixWorkspace_sptr shiftedCan =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            shiftedCanName);
    if (m_uiForm.ckShiftCan->isChecked()) {
      IAlgorithm_sptr scaleX = AlgorithmManager::Instance().create("ScaleX");
      scaleX->initialize();
      scaleX->setProperty("InputWorkspace", shiftedCan);
      scaleX->setProperty("OutputWorkspace", shiftedCanName);
      scaleX->setProperty("Factor", m_uiForm.spCanShift->value());
      scaleX->setProperty("Operation", "Add");
      scaleX->execute();
      IAlgorithm_sptr rebin =
          AlgorithmManager::Instance().create("RebinToWorkspace");
      rebin->initialize();
      rebin->setProperty("WorkspaceToRebin", shiftedCan);
      rebin->setProperty("WorkspaceToMatch", sampleWsName.toStdString());
      rebin->setProperty("OutputWorkspace", shiftedCanName);
      rebin->execute();
    }
    absCorAlgo->setProperty("CanWorkspace", shiftedCanName);

    bool useCanCorrections = m_uiForm.ckUseCanCorrections->isChecked();
    absCorAlgo->setProperty("UseCanCorrections", useCanCorrections);

    if (useCanCorrections) {

      absCorAlgo->setProperty(
          "CanDensityType", m_uiForm.cbCanDensity->currentText().toStdString());
      absCorAlgo->setProperty("CanDensity", m_uiForm.spCanDensity->value());

      QString canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
      absCorAlgo->setProperty("CanChemicalFormula",
                              canChemicalFormula.toStdString());
    }

    addShapeSpecificCanOptions(absCorAlgo, sampleShape);
  }

  // Generate workspace names
  int nameCutIndex = sampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = sampleWsName.length();

  QString outputBaseName = sampleWsName.left(nameCutIndex);

  QString outputWsName = outputBaseName + "_" + sampleShape + "_red";
  absCorAlgo->setProperty("OutputWorkspace", outputWsName.toStdString());

  // Set the correction workspace to keep the factors if desired
  bool keepCorrectionFactors = m_uiForm.ckKeepFactors->isChecked();
  QString outputFactorsWsName = outputBaseName + "_" + sampleShape + "_Factors";
  if (keepCorrectionFactors)
    absCorAlgo->setProperty("CorrectionsWorkspace",
                            outputFactorsWsName.toStdString());

  // Add correction algorithm to batch
  m_batchAlgoRunner->addAlgorithm(absCorAlgo);

  m_absCorAlgo = absCorAlgo;
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
    alg->setProperty("SampleHeight", sampleHeight);

    double sampleWidth = m_uiForm.spFlatSampleWidth->value();
    alg->setProperty("SampleWidth", sampleWidth);

    double sampleThickness = m_uiForm.spFlatSampleThickness->value();
    alg->setProperty("SampleThickness", sampleThickness);

    double sampleAngle = m_uiForm.spFlatSampleAngle->value();
    alg->setProperty("SampleAngle", sampleAngle);

  } else if (shape == "Annulus") {
    double sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
    alg->setProperty("SampleInnerRadius", sampleInnerRadius);

    double sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
    alg->setProperty("SampleOuterRadius", sampleOuterRadius);

    double canInnerRadius = m_uiForm.spAnnCanInnerRadius->value();
    alg->setProperty("CanInnerRadius", canInnerRadius);

    double canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
    alg->setProperty("CanOuterRadius", canOuterRadius);

  } else if (shape == "Cylinder") {
    double sampleRadius = m_uiForm.spCylSampleRadius->value();
    alg->setProperty("SampleRadius", sampleRadius);

    double sampleHeight = m_uiForm.spCylSampleHeight->value();
    alg->setProperty("SampleHeight", sampleHeight);
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
    alg->setProperty("CanFrontThickness", canFrontThickness);

    double canBackThickness = m_uiForm.spFlatCanBackThickness->value();
    alg->setProperty("CanBackThickness", canBackThickness);
  } else if (shape == "Cylinder") {
    double canRadius = m_uiForm.spCylCanRadius->value();
    alg->setProperty("CanRadius", canRadius);
  }
}

bool AbsorptionCorrections::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

  if (uiv.checkFieldIsNotEmpty("Sample Chemical Formula",
                               m_uiForm.leSampleChemicalFormula))
    uiv.checkFieldIsValid("Sample Chemical Formula",
                          m_uiForm.leSampleChemicalFormula);
  const auto sampleChem =
      m_uiForm.leSampleChemicalFormula->text().toStdString();
  const auto containerChem =
      m_uiForm.leCanChemicalFormula->text().toStdString();
  try {
    Mantid::Kernel::Material::parseChemicalFormula(sampleChem);
  } catch (std::runtime_error &ex) {
    UNUSED_ARG(ex);
    uiv.addErrorMessage("Chemical Formula for Sample was not recognised.");
  }
  try {
    Mantid::Kernel::Material::parseChemicalFormula(containerChem);
  } catch (std::runtime_error &ex) {
    UNUSED_ARG(ex);
    uiv.addErrorMessage("Chemical Formula for Container was not recognised.");
  }

  bool useCan = m_uiForm.ckUseCan->isChecked();
  if (useCan) {
    uiv.checkDataSelectorIsValid("Container", m_uiForm.dsCanInput);

    bool useCanCorrections = m_uiForm.ckUseCanCorrections->isChecked();
    if (useCanCorrections) {
      if (uiv.checkFieldIsNotEmpty("Container Chemical Formula",
                                   m_uiForm.leCanChemicalFormula))
        uiv.checkFieldIsValid("Container Chemical Formula",
                              m_uiForm.leCanChemicalFormula);
    }
  }

  // Give error for failed validation
  if (!uiv.isAllInputValid()) {
    QString error = uiv.generateErrorMessage();
    showMessageBox(error);
  }

  return uiv.isAllInputValid();
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
  }
  if (m_uiForm.ckShiftCan->isChecked()) {
    IAlgorithm_sptr shiftLog =
        AlgorithmManager::Instance().create("AddSampleLog");
    shiftLog->initialize();
    shiftLog->setProperty("Workspace", m_pythonExportWsName);
    shiftLog->setProperty("LogName", "container_shift");
    shiftLog->setProperty("logType", "Number");
    shiftLog->setProperty("LogText", boost::lexical_cast<std::string>(
                                         m_uiForm.spCanShift->value()));
    shiftLog->execute();
  }
  // Enable plot and save
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

void AbsorptionCorrections::getBeamDefaults(const QString &dataName) {
  auto sampleWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      dataName.toStdString());

  if (!sampleWs) {
    g_log.warning() << "Failed to find workspace " << dataName.toStdString()
                    << "\n";
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

  if (m_uiForm.ckKeepFactors->isChecked()) {
    std::string factorsWs =
        m_absCorAlgo->getPropertyValue("CorrectionsWorkspace");
    if (checkADSForPlotSaveWorkspace(factorsWs, false))
      addSaveWorkspaceToQueue(QString::fromStdString(factorsWs));
  }
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle mantid plotting
 */
void AbsorptionCorrections::plotClicked() {

  QStringList plotData = {QString::fromStdString(m_pythonExportWsName),
                          m_uiForm.dsSampleInput->getCurrentDataName()};
  auto outputFactorsWsName =
      m_absCorAlgo->getPropertyValue("CorrectionsWorkspace");
  if (m_uiForm.ckKeepFactors->isChecked()) {
    QStringList plotCorr = {QString::fromStdString(outputFactorsWsName) +
                            "_ass"};
    if (m_uiForm.ckUseCanCorrections->isChecked()) {
      plotCorr.push_back(QString::fromStdString(outputFactorsWsName) + "_acc");
      QString shiftedWs = QString::fromStdString(
          m_absCorAlgo->getPropertyValue("CanWorkspace"));
      plotData.push_back(shiftedWs);
    }
    plotSpectrum(plotCorr, 0);
  }
  plotSpectrum(plotData, 0);
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
