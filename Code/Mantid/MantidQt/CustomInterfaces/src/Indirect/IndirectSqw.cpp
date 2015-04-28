#include "MantidQtCustomInterfaces/Indirect/IndirectSqw.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectSqw::IndirectSqw(IndirectDataReduction * idrUI, QWidget * parent) :
    IndirectDataReductionTab(idrUI, parent)
  {
    m_uiForm.setupUi(parent);

    connect(m_uiForm.dsSampleInput, SIGNAL(loadClicked()), this, SLOT(plotContour()));
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(sqwAlgDone(bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectSqw::~IndirectSqw()
  {
  }

  void IndirectSqw::setup()
  {
  }

  bool IndirectSqw::validate()
  {
    double tolerance = 1e-10;
    UserInputValidator uiv;

    // Validate the data selector
    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

    // Validate Q binning
    uiv.checkBins(m_uiForm.spQLow->value(), m_uiForm.spQWidth->value(), m_uiForm.spQHigh->value(), tolerance);

    // If selected, validate energy binning
    if(m_uiForm.ckRebinInEnergy->isChecked())
      uiv.checkBins(m_uiForm.spELow->value(), m_uiForm.spEWidth->value(), m_uiForm.spEHigh->value(), tolerance);

    QString errorMessage = uiv.generateErrorMessage();

    // Show an error message if needed
    if(!errorMessage.isEmpty())
      emit showMessageBox(errorMessage);

    return errorMessage.isEmpty();
  }

  void IndirectSqw::run()
  {
    QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
    QString sqwWsName = sampleWsName.left(sampleWsName.length() - 4) + "_sqw";
    QString eRebinWsName = sampleWsName.left(sampleWsName.length() - 4) + "_r";

    QString rebinString = m_uiForm.spQLow->text() + "," + m_uiForm.spQWidth->text() +
      "," + m_uiForm.spQHigh->text();

    // Rebin in energy
    bool rebinInEnergy = m_uiForm.ckRebinInEnergy->isChecked();
    if(rebinInEnergy)
    {
      QString eRebinString = m_uiForm.spELow->text() + "," + m_uiForm.spEWidth->text() +
                             "," + m_uiForm.spEHigh->text();

      IAlgorithm_sptr energyRebinAlg = AlgorithmManager::Instance().create("Rebin");
      energyRebinAlg->initialize();

      energyRebinAlg->setProperty("InputWorkspace", sampleWsName.toStdString());
      energyRebinAlg->setProperty("OutputWorkspace", eRebinWsName.toStdString());
      energyRebinAlg->setProperty("Params", eRebinString.toStdString());

      m_batchAlgoRunner->addAlgorithm(energyRebinAlg);
    }

    // Get correct S(Q, w) algorithm
    QString eFixed = getInstrumentDetails()["Efixed"];

    IAlgorithm_sptr sqwAlg;
    QString rebinType = m_uiForm.cbRebinType->currentText();

    if(rebinType == "Parallelepiped (SofQW2)")
      sqwAlg = AlgorithmManager::Instance().create("SofQW2");
    else if(rebinType == "Parallelepiped/Fractional Area (SofQW3)")
      sqwAlg = AlgorithmManager::Instance().create("SofQW3");

    // S(Q, w) algorithm
    sqwAlg->initialize();

    BatchAlgorithmRunner::AlgorithmRuntimeProps sqwInputProps;
    if(rebinInEnergy)
      sqwInputProps["InputWorkspace"] = eRebinWsName.toStdString();
    else
      sqwInputProps["InputWorkspace"] = sampleWsName.toStdString();

    sqwAlg->setProperty("OutputWorkspace", sqwWsName.toStdString());
    sqwAlg->setProperty("QAxisBinning", rebinString.toStdString());
    sqwAlg->setProperty("EMode", "Indirect");
    sqwAlg->setProperty("EFixed", eFixed.toStdString());

    m_batchAlgoRunner->addAlgorithm(sqwAlg, sqwInputProps);

    // Add sample log for S(Q, w) algorithm used
    IAlgorithm_sptr sampleLogAlg = AlgorithmManager::Instance().create("AddSampleLog");
    sampleLogAlg->initialize();

    sampleLogAlg->setProperty("LogName", "rebin_type");
    sampleLogAlg->setProperty("LogType", "String");
    sampleLogAlg->setProperty("LogText", rebinType.toStdString());

    BatchAlgorithmRunner::AlgorithmRuntimeProps inputToAddSampleLogProps;
    inputToAddSampleLogProps["Workspace"] = sqwWsName.toStdString();

    m_batchAlgoRunner->addAlgorithm(sampleLogAlg, inputToAddSampleLogProps);

    // Save S(Q, w) workspace
    if(m_uiForm.ckSave->isChecked())
    {
      QString saveFilename = sqwWsName + ".nxs";

      IAlgorithm_sptr saveNexusAlg = AlgorithmManager::Instance().create("SaveNexus");
      saveNexusAlg->initialize();

      saveNexusAlg->setProperty("Filename", saveFilename.toStdString());

      BatchAlgorithmRunner::AlgorithmRuntimeProps inputToSaveNexusProps;
      inputToSaveNexusProps["InputWorkspace"] = sqwWsName.toStdString();

      m_batchAlgoRunner->addAlgorithm(saveNexusAlg, inputToSaveNexusProps);
    }

    // Set the name of the result workspace for Python export
    m_pythonExportWsName = sqwWsName.toStdString();

    m_batchAlgoRunner->executeBatch();
  }

  /**
   * Handles plotting the S(Q, w) workspace when the algorithm chain is finished.
   *
   * @param error If the algorithm chain failed
   */
  void IndirectSqw::sqwAlgDone(bool error)
  {
    if(error)
      return;

    // Get the workspace name
    QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
    QString sqwWsName = sampleWsName.left(sampleWsName.length() - 4) + "_sqw";

    QString plotType = m_uiForm.cbPlotType->currentText();

    if(plotType == "Contour")
      plot2D(sqwWsName);

    else if(plotType == "Spectra")
    {
      auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sqwWsName.toStdString());
      int numHist = static_cast<int>(ws->getNumberHistograms());
      plotSpectrum(sqwWsName, 0, numHist);
    }
  }

  /**
   * Handles the Plot Input button
   *
   * Creates a colour 2D plot of the data
   */
  void IndirectSqw::plotContour()
  {
    if(m_uiForm.dsSampleInput->isValid())
    {
      QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();

      QString convertedWsName = sampleWsName.left(sampleWsName.length() - 4) + "_rqw";

      IAlgorithm_sptr convertSpecAlg = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
      convertSpecAlg->initialize();

      convertSpecAlg->setProperty("InputWorkspace", sampleWsName.toStdString());
      convertSpecAlg->setProperty("OutputWorkspace", convertedWsName.toStdString());
      convertSpecAlg->setProperty("Target", "ElasticQ");
      convertSpecAlg->setProperty("EMode", "Indirect");

      convertSpecAlg->execute();

      QString pyInput = "plot2D('" + convertedWsName + "')\n";
      m_pythonRunner.runPythonCode(pyInput);
    }
    else
    {
      emit showMessageBox("Invalid filename.");
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
