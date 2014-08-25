#include "MantidQtCustomInterfaces/IndirectSqw.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>

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
    connect(m_uiForm.sqw_ckRebinE, SIGNAL(toggled(bool)), this, SLOT(sOfQwRebinE(bool)));
    connect(m_uiForm.sqw_dsSampleInput, SIGNAL(loadClicked()), this, SLOT(sOfQwPlotInput()));

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

  bool IndirectSqw::validate()
  {
    bool valid = true;

    UserInputValidator uiv;
    uiv.checkDataSelectorIsValid("Sample", m_uiForm.sqw_dsSampleInput);
    QString error = uiv.generateErrorMessage();

    if (!error.isEmpty())
    {
      valid = false;
      emit showMessageBox(error);
    }

    if ( m_uiForm.sqw_ckRebinE->isChecked() )
    {
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
    }

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
   * Enabled/disables the rebin in energy UI widgets
   *
   * @param state :: True to enable RiE UI, false to disable
   */
  void IndirectSqw::sOfQwRebinE(bool state)
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
  void IndirectSqw::sOfQwPlotInput()
  {
    QString pyInput = "from mantid.simpleapi import *\n"
      "from mantidplot import *\n";

    if (m_uiForm.sqw_dsSampleInput->isValid())
    {
      if(m_uiForm.sqw_dsSampleInput->isFileSelectorVisible())
      {
        //Load file into workspacwe
        pyInput += "filename = r'" + m_uiForm.sqw_dsSampleInput->getFullFilePath() + "'\n"
          "(dir, file) = os.path.split(filename)\n"
          "(sqwInput, ext) = os.path.splitext(file)\n"
          "LoadNexus(Filename=filename, OutputWorkspace=sqwInput)\n";
      }
      else
      {
        //Use existing workspace
        pyInput += "sqwInput = '" + m_uiForm.sqw_dsSampleInput->getCurrentDataName() + "'\n";
      }

      pyInput += "ConvertSpectrumAxis(InputWorkspace=sqwInput, OutputWorkspace=sqwInput[:-4]+'_rqw', Target='ElasticQ', EMode='Indirect')\n"
        "ws = importMatrixWorkspace(sqwInput[:-4]+'_rqw')\n"
        "ws.plotGraph2D()\n";

      QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();
    }
    else
    {
      emit showMessageBox("Invalid filename.");
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
