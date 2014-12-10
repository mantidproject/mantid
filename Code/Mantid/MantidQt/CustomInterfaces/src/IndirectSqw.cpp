#include "MantidQtCustomInterfaces/IndirectSqw.h"

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
  IndirectSqw::IndirectSqw(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
    m_uiForm.sqw_leELow->setValidator(m_valDbl);
    m_uiForm.sqw_leEWidth->setValidator(m_valDbl);
    m_uiForm.sqw_leEHigh->setValidator(m_valDbl);
    m_uiForm.sqw_leQLow->setValidator(m_valDbl);
    m_uiForm.sqw_leQWidth->setValidator(m_valDbl);
    m_uiForm.sqw_leQHigh->setValidator(m_valDbl);

    connect(m_uiForm.sqw_ckRebinE, SIGNAL(toggled(bool)), this, SLOT(energyRebinToggle(bool)));
    connect(m_uiForm.sqw_dsSampleInput, SIGNAL(loadClicked()), this, SLOT(plotContour()));

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
    bool valid = true;

    UserInputValidator uiv;
    uiv.checkDataSelectorIsValid("Sample", m_uiForm.sqw_dsSampleInput);
    QString error = uiv.generateErrorMessage();

    if(!error.isEmpty())
    {
      valid = false;
      emit showMessageBox(error);
    }

    if(m_uiForm.sqw_ckRebinE->isChecked() && !validateEnergyRebin())
      valid = false;

    if(!validateQRebin())
      valid = false;

    return valid;
  }

  /**
   * Validates the Q rebinning parameters.
   *
   * @returns If the rebinning is valid
   */
  bool IndirectSqw::validateQRebin()
  {
    bool valid = true;

    if ( m_uiForm.sqw_leQLow->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valQLow->setText("*");
    }
    else
    {
      m_uiForm.sqw_valQLow->setText(" ");
    }

    if ( m_uiForm.sqw_leQWidth->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valQWidth->setText("*");
    }
    else
    {
      m_uiForm.sqw_valQWidth->setText(" ");
    }

    if ( m_uiForm.sqw_leQHigh->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valQHigh->setText("*");
    }
    else
    {
      m_uiForm.sqw_valQHigh->setText(" ");
    }

    return valid;
  }

  /**
   * Validates the energy rebinning parameters.
   *
   * @returns If the rebinning is valid
   */
  bool IndirectSqw::validateEnergyRebin()
  {
    bool valid = true;

    if ( m_uiForm.sqw_leELow->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valELow->setText("*");
    }
    else
    {
      m_uiForm.sqw_valELow->setText(" ");
    }

    if ( m_uiForm.sqw_leEWidth->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valEWidth->setText("*");
    }
    else
    {
      m_uiForm.sqw_valEWidth->setText(" ");
    }

    if ( m_uiForm.sqw_leEHigh->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valEHigh->setText("*");
    }
    else
    {
      m_uiForm.sqw_valEHigh->setText(" ");
    }

    return valid;
  }

  void IndirectSqw::run()
  {
    QString sampleWsName = m_uiForm.sqw_dsSampleInput->getCurrentDataName();
    QString sqwWsName = sampleWsName.left(sampleWsName.length() - 4) + "_sqw";
    QString eRebinWsName = sampleWsName.left(sampleWsName.length() - 4) + "_r";

    QString rebinString = m_uiForm.sqw_leQLow->text() + "," + m_uiForm.sqw_leQWidth->text() +
      "," + m_uiForm.sqw_leQHigh->text();

    // Rebin in energy
    bool rebinInEnergy = m_uiForm.sqw_ckRebinE->isChecked();
    if(rebinInEnergy)
    {
      QString eRebinString = m_uiForm.sqw_leELow->text() + "," + m_uiForm.sqw_leEWidth->text() +
                             "," + m_uiForm.sqw_leEHigh->text();

      IAlgorithm_sptr energyRebinAlg = AlgorithmManager::Instance().create("Rebin");
      energyRebinAlg->initialize();

      energyRebinAlg->setProperty("InputWorkspace", sampleWsName.toStdString());
      energyRebinAlg->setProperty("OutputWorkspace", eRebinWsName.toStdString());
      energyRebinAlg->setProperty("Params", eRebinString.toStdString());

      m_batchAlgoRunner->addAlgorithm(energyRebinAlg);
    }

    // Get correct S(Q, w) algorithm
    QString eFixed = getInstrumentDetails()["efixed-val"];

    IAlgorithm_sptr sqwAlg;
    QString rebinType = m_uiForm.sqw_cbRebinType->currentText();

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
    if(m_uiForm.sqw_ckSave->isChecked())
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
    QString sampleWsName = m_uiForm.sqw_dsSampleInput->getCurrentDataName();
    QString sqwWsName = sampleWsName.left(sampleWsName.length() - 4) + "_sqw";

    QString pyInput = "sqw_ws = '" + sqwWsName + "'\n";
    QString plotType = m_uiForm.sqw_cbPlotType->currentText();

    if(plotType == "Contour")
    {
      pyInput += "plot2D(sqw_ws)\n";
    }

    else if(plotType == "Spectra")
    {
      pyInput +=
        "n_spec = mtd[sqw_ws].getNumberHistograms()\n"
        "plotSpectrum(sqw_ws, range(0, n_spec))\n";
    }

    m_pythonRunner.runPythonCode(pyInput);
  }

  /**
   * Enabled/disables the rebin in energy UI widgets
   *
   * @param state :: True to enable RiE UI, false to disable
   */
  void IndirectSqw::energyRebinToggle(bool state)
  {
    QString val;
    if(state)
      val = "*";
    else
      val = " ";

    m_uiForm.sqw_leELow->setEnabled(state);
    m_uiForm.sqw_leEWidth->setEnabled(state);
    m_uiForm.sqw_leEHigh->setEnabled(state);
    m_uiForm.sqw_valELow->setEnabled(state);
    m_uiForm.sqw_valELow->setText(val);
    m_uiForm.sqw_valEWidth->setEnabled(state);
    m_uiForm.sqw_valEWidth->setText(val);
    m_uiForm.sqw_valEHigh->setEnabled(state);
    m_uiForm.sqw_valEHigh->setText(val);
    m_uiForm.sqw_lbELow->setEnabled(state);
    m_uiForm.sqw_lbEWidth->setEnabled(state);
    m_uiForm.sqw_lbEHigh->setEnabled(state);
  }

  /**
   * Handles the Plot Input button
   *
   * Creates a colour 2D plot of the data
   */
  void IndirectSqw::plotContour()
  {
    if(m_uiForm.sqw_dsSampleInput->isValid())
    {
      QString sampleWsName = m_uiForm.sqw_dsSampleInput->getCurrentDataName();

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
