#include "MantidQtCustomInterfaces/IndirectSqw.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>

using namespace Mantid::API;

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
    connect(m_uiForm.sqw_ckRebinE, SIGNAL(toggled(bool)), this, SLOT(energyRebinToggle(bool)));
    connect(m_uiForm.sqw_dsSampleInput, SIGNAL(loadClicked()), this, SLOT(plotContour()));

    m_uiForm.sqw_leELow->setValidator(m_valDbl);
    m_uiForm.sqw_leEWidth->setValidator(m_valDbl);
    m_uiForm.sqw_leEHigh->setValidator(m_valDbl);
    m_uiForm.sqw_leQLow->setValidator(m_valDbl);
    m_uiForm.sqw_leQWidth->setValidator(m_valDbl);
    m_uiForm.sqw_leQHigh->setValidator(m_valDbl);
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

    if(m_uiForm.sqw_ckRebinE->isChecked() && validateEnergyRebin())
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
    QString rebinString = m_uiForm.sqw_leQLow->text() + "," + m_uiForm.sqw_leQWidth->text() +
      "," + m_uiForm.sqw_leQHigh->text();

    QString wsname;
    if(m_uiForm.sqw_dsSampleInput->isFileSelectorVisible())
    {
      // Load Nexus file into workspace
      QString filename = m_uiForm.sqw_dsSampleInput->getFullFilePath();
      QFileInfo fi(filename);
      wsname = fi.baseName();

      if(!loadFile(filename, wsname))
      {
        emit showMessageBox("Could not load Nexus file");
      }
    }
    else
    {
      // Get the workspace
      wsname = m_uiForm.sqw_dsSampleInput->getCurrentDataName();
    }

    QString pyInput = "from mantid.simpleapi import *\n";

    // Create output name before rebinning
    pyInput += "sqwInput = '" + wsname + "'\n";
    pyInput += "sqwOutput = sqwInput[:-3] + 'sqw'\n";

    if ( m_uiForm.sqw_ckRebinE->isChecked() )
    {
      QString eRebinString = m_uiForm.sqw_leELow->text() + "," + m_uiForm.sqw_leEWidth->text() +
        "," + m_uiForm.sqw_leEHigh->text();

      pyInput += "Rebin(InputWorkspace=sqwInput, OutputWorkspace=sqwInput+'_r', Params='" + eRebinString + "')\n"
        "sqwInput += '_r'\n";
    }

    pyInput +=
      "efixed = " + m_uiForm.leEfixed->text() + "\n"
      "rebin = '" + rebinString + "'\n";

    QString rebinType = m_uiForm.sqw_cbRebinType->currentText();
    if(rebinType == "Centre (SofQW)")
      pyInput += "SofQW(InputWorkspace=sqwInput, OutputWorkspace=sqwOutput, QAxisBinning=rebin, EMode='Indirect', EFixed=efixed)\n";
    else if(rebinType == "Parallelepiped (SofQW2)")
      pyInput += "SofQW2(InputWorkspace=sqwInput, OutputWorkspace=sqwOutput, QAxisBinning=rebin, EMode='Indirect', EFixed=efixed)\n";
    else if(rebinType == "Parallelepiped/Fractional Area (SofQW3)")
      pyInput += "SofQW3(InputWorkspace=sqwInput, OutputWorkspace=sqwOutput, QAxisBinning=rebin, EMode='Indirect', EFixed=efixed)\n";

    pyInput += "AddSampleLog(Workspace=sqwOutput, LogName='rebin_type', LogType='String', LogText='"+rebinType+"')\n";

    if ( m_uiForm.sqw_ckSave->isChecked() )
    {
      pyInput += "SaveNexus(InputWorkspace=sqwOutput, Filename=sqwOutput+'.nxs')\n";

      if (m_uiForm.sqw_ckVerbose->isChecked())
      {
        pyInput += "logger.notice(\"Resolution file saved to default save directory.\")\n";
      }
    }

    if ( m_uiForm.sqw_cbPlotType->currentText() == "Contour" )
    {
      pyInput += "importMatrixWorkspace(sqwOutput).plotGraph2D()\n";
    }

    else if ( m_uiForm.sqw_cbPlotType->currentText() == "Spectra" )
    {
      pyInput +=
        "nspec = mtd[sqwOutput].getNumberHistograms()\n"
        "plotSpectrum(sqwOutput, range(0, nspec))\n";
    }

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();
  }

  /**
   * Enabled/disables the rebin in energy UI widgets
   *
   * @param state :: True to enable RiE UI, false to disable
   */
  void IndirectSqw::energyRebinToggle(bool state)
  {
    QString val;
    if ( state ) val = "*";
    else val = " ";
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
    if (m_uiForm.sqw_dsSampleInput->isValid())
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

      QString pyInput = "from mantidplot import plot2D\n"
                        "plot2D('" + convertedWsName + "')\n";

      m_pythonRunner.runPythonCode(pyInput).trimmed();
    }
    else
    {
      emit showMessageBox("Invalid filename.");
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
