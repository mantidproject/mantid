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


  //TODO
  void CalcCorr::run()
  {
    // Get correct corrections algorithm
    QString sampleShape = m_uiForm.cbSampleShape->currentText();
    QString algorithmName = sampleShape.replace(" ", "") + "PaalmanPingsCorrection";

    IAlgorithm_sptr absCorAlgo = AlgorithmManager::Instance().create(algorithmName.toStdString());
    absCorAlgo->initialize();

    // Sample details
    QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
    absCorAlgo->setProperty("SampleWorkspace", sampleWsName.toStdString());

    double sampleNumberDensity = m_uiForm.spSampleNumberDensity->value();
    absCorAlgo->setProperty("SampleNumberDensity", sampleNumberDensity);

    QString sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
    absCorAlgo->setProperty("SampleChemicalFormula", sampleChemicalFormula.toStdString());

    //TODO: sample options

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

      //TODO: can options
    }

    //TODO: emode, efixed

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

    QString outputWsName = sampleWsName.left(nameCutIndex) + "_" + correctionType + "_abs";
    absCorAlgo->setProperty("OutputWorkspace", outputWsName.toStdString());

    // Run corrections algorithm
    m_batchAlgoRunner->addAlgorithm(absCorAlgo);
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

    if(uiv.checkFieldIsNotEmpty("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula, m_uiForm.valSampleChemicalFormula))
      uiv.checkFieldIsValid("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula, m_uiForm.valSampleChemicalFormula);

    bool useCan = m_uiForm.ckUseCan->isChecked();
    if(useCan)
    {
      uiv.checkDataSelectorIsValid("Can", m_uiForm.dsContainer);

      if(uiv.checkFieldIsNotEmpty("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula))
        uiv.checkFieldIsValid("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula);

      QString sample = m_uiForm.dsSample->getCurrentDataName();
      QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
      QString container = m_uiForm.dsContainer->getCurrentDataName();
      QString containerType = container.right(container.length() - container.lastIndexOf("_"));

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
    }

    //TODO: plot
  }


  void CalcCorr::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSample->readSettings(settings.group());
    m_uiForm.dsContainer->readSettings(settings.group());
  }


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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
