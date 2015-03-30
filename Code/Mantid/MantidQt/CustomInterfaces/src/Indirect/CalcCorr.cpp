#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/Indirect/CalcCorr.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"

#include <QLineEdit>
#include <QList>
#include <QValidator>
#include <QDoubleValidator>
#include <QRegExpValidator>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("CalcCorr");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  CalcCorr::CalcCorr(QWidget * parent) :
    IDATab(parent)
  {
    m_uiForm.setupUi(parent);

    connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(getBeamWidthFromWorkspace(const QString&)));
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

    QRegExp regex("[A-Za-z0-9\\-\\(\\)]*");
    QValidator *formulaValidator = new QRegExpValidator(regex, this);
    m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
    m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);
  }


  void CalcCorr::setup()
  {
    doValidation(true);
  }


  void CalcCorr::run()
  {
    //TODO: check sample binning matches can binning, ask if should rebin to match

    // Get correct corrections algorithm
    QString sampleShape = m_uiForm.cbSampleShape->currentText();
    QString algorithmName = sampleShape.replace(" ", "") + "PaalmanPingsCorrection";

    IAlgorithm_sptr absCorAlgo = AlgorithmManager::Instance().create(algorithmName.toStdString());
    absCorAlgo->initialize();

    // Sample details
    QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
    MatrixWorkspace_sptr sampleWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sampleWsName.toStdString());
    absCorAlgo->setProperty("SampleWorkspace", sampleWsName.toStdString());

    double sampleNumberDensity = m_uiForm.spSampleNumberDensity->value();
    absCorAlgo->setProperty("SampleNumberDensity", sampleNumberDensity);

    QString sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
    absCorAlgo->setProperty("SampleChemicalFormula", sampleChemicalFormula.toStdString());

    addShapeSpecificSampleOptions(absCorAlgo, sampleShape);

    // Can details
    bool useCan = m_uiForm.ckUseCan->isChecked();
    if(useCan)
    {
      QString canWsName = m_uiForm.dsContainer->getCurrentDataName();
      absCorAlgo->setProperty("CanWorkspace", canWsName.toStdString());

      double canNumberDensity = m_uiForm.spCanNumberDensity->value();
      absCorAlgo->setProperty("CanNumberDensity", canNumberDensity);

      QString canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
      absCorAlgo->setProperty("CanChemicalFormula", canChemicalFormula.toStdString());

      addShapeSpecificCanOptions(absCorAlgo, sampleShape);
    }

    absCorAlgo->setProperty("EMode", getEMode(sampleWs));
    absCorAlgo->setProperty("EFixed", getEFixed(sampleWs));

    // Generate workspace names
    int nameCutIndex = sampleWsName.lastIndexOf("_");
    if(nameCutIndex == -1)
      nameCutIndex = sampleWsName.length();

    QString correctionType;
    switch(m_uiForm.cbSampleShape->currentIndex())
    {
      case 0:
        correctionType = "flt";
        break;
      case 1:
        correctionType = "cyl";
        break;
    }

    const QString outputWsName = sampleWsName.left(nameCutIndex) + "_" + correctionType + "_abs";
    absCorAlgo->setProperty("OutputWorkspace", outputWsName.toStdString());

    // Add corrections algorithm to queue
    m_batchAlgoRunner->addAlgorithm(absCorAlgo);

    // Add save algorithms if required
    bool save = m_uiForm.ckSave->isChecked();
    if(save)
      addSaveWorkspaceToQueue(outputWsName);

    // Run algorithm queue
    m_batchAlgoRunner->executeBatchAsync();

    // Set the result workspace for Python script export
    m_pythonExportWsName = outputWsName.toStdString();
  }


  bool CalcCorr::validate()
  {
    return doValidation();
  }


  /**
   * Does validation on the user input.
   *
   * @param silent Set to true to avoid creating an error message
   * @return True if all user input is valid
   */
  bool CalcCorr::doValidation(bool silent)
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

    // Validate chemical formula
    if(uiv.checkFieldIsNotEmpty("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula, m_uiForm.valSampleChemicalFormula))
      uiv.checkFieldIsValid("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula, m_uiForm.valSampleChemicalFormula);

    bool useCan = m_uiForm.ckUseCan->isChecked();
    if(useCan)
    {
      uiv.checkDataSelectorIsValid("Can", m_uiForm.dsContainer);

      // Validate chemical formula
      if(uiv.checkFieldIsNotEmpty("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula))
        uiv.checkFieldIsValid("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula);

      // Ensure sample and container are the same kind of data
      QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
      QString sampleType = sampleWsName.right(sampleWsName.length() - sampleWsName.lastIndexOf("_"));
      QString containerWsName = m_uiForm.dsContainer->getCurrentDataName();
      QString containerType = containerWsName.right(containerWsName.length() - containerWsName.lastIndexOf("_"));

      g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
      g_log.debug() << "Can type is: " << containerType.toStdString() << std::endl;

      if(containerType != sampleType)
        uiv.addErrorMessage("Sample and can workspaces must contain the same type of data.");
    }

    // Show error mssage if needed
    if(!uiv.isAllInputValid() && !silent)
      emit showMessageBox(uiv.generateErrorMessage());

    return uiv.isAllInputValid();
  }


  /**
   * Handles completion of the correction algorithm.
   *
   * @param error True of the algorithm failed
   */
  void CalcCorr::algorithmComplete(bool error)
  {
    if(error)
    {
      emit showMessageBox("Absorption correction calculation failed.\nSee Results Log for more details.");
      return;
    }

    // Handle Mantid plotting
    bool plot = m_uiForm.ckPlotOutput->isChecked();
    if(plot)
      plotSpectrum(QString::fromStdString(m_pythonExportWsName));
  }


  void CalcCorr::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSample->readSettings(settings.group());
    m_uiForm.dsContainer->readSettings(settings.group());
  }


  /**
   * Gets the beam width from the instrument parameters on a given workspace
   * and update the relevant options on the UI.
   *
   * @param wsName Name of the workspace
   */
  void CalcCorr::getBeamWidthFromWorkspace(const QString& wsName)
  {
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());

    if(!ws)
    {
      g_log.warning() << "Failed to find workspace " << wsName.toStdString() << std::endl;
      return;
    }

    std::string paramName = "Workflow.beam-width";
    auto instrument = ws->getInstrument();

    if(instrument->hasParameter(paramName))
    {
      QString beamWidth = QString::fromStdString(instrument->getStringParameter(paramName)[0]);
      double beamWidthValue = beamWidth.toDouble();
      m_uiForm.spCylBeamWidth->setValue(beamWidthValue);
      m_uiForm.spCylBeamHeight->setValue(beamWidthValue);
    }
  }


  /**
   * Sets algorithm properties specific to the sample for a given shape.
   *
   * @param alg Algorithm to set properties of
   * @param shape Sample shape
   */
  void CalcCorr::addShapeSpecificSampleOptions(IAlgorithm_sptr alg, QString shape)
  {
    if(shape == "FlatPlate")
    {
      double sampleThickness = m_uiForm.spFlatSampleThickness->value();
      alg->setProperty("SampleThickness", sampleThickness);

      double sampleAngle = m_uiForm.spFlatSampleAngle->value();
      alg->setProperty("SampleAngle", sampleAngle);
    }
    else if(shape == "Cylinder")
    {
      double sampleInnerRadius = m_uiForm.spCylSampleInnerRadius->value();
      alg->setProperty("SampleInnerRadius", sampleInnerRadius);

      double sampleOuterRadius = m_uiForm.spCylSampleOuterRadius->value();
      alg->setProperty("SampleOuterRadius", sampleOuterRadius);

      double beamWidth = m_uiForm.spCylBeamWidth->value();
      alg->setProperty("BeamWidth", beamWidth);

      double beamHeight = m_uiForm.spCylBeamHeight->value();
      alg->setProperty("BeamHeight", beamHeight);

      double stepSize = m_uiForm.spCylStepSize->value();
      alg->setProperty("StepSize", stepSize);
    }
  }


  /**
   * Sets algorithm properties specific to the container for a given shape.
   *
   * @param alg Algorithm to set properties of
   * @param shape Sample shape
   */
  void CalcCorr::addShapeSpecificCanOptions(IAlgorithm_sptr alg, QString shape)
  {
    if(shape == "FlatPlate")
    {
      double canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
      alg->setProperty("CanFrontThickness", canFrontThickness);

      double canBackThickness = m_uiForm.spFlatCanBackThickness->value();
      alg->setProperty("SampleThickness", canBackThickness);
    }
    else if(shape == "Cylinder")
    {
      double canOuterRadius = m_uiForm.spCylCanOuterRadius->value();
      alg->setProperty("CanOuterRadius", canOuterRadius);
    }
  }


} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
