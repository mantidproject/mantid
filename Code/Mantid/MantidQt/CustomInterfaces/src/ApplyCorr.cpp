#include "MantidQtCustomInterfaces/ApplyCorr.h"
#include "MantidAPI/AnalysisDataService.h"

#include <QStringList>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  ApplyCorr::ApplyCorr(QWidget * parent) : 
    IDATab(parent)
  {

  }

  void ApplyCorr::setup()
  {
    connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_dsContainer, SLOT(setEnabled(bool)));
    connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_ckScaleMultiplier, SLOT(setEnabled(bool)));
    connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), this, SLOT(scaleMultiplierCheck(bool)));
    connect(uiForm().abscor_ckUseCorrections, SIGNAL(toggled(bool)), uiForm().abscor_dsCorrections, SLOT(setEnabled(bool)));
    connect(uiForm().abscor_ckScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(scaleMultiplierCheck(bool)));
    connect(uiForm().abscor_cbGeometry, SIGNAL(currentIndexChanged(int)), this, SLOT(handleGeometryChange(int)));

    // Create a validator for input box of the Scale option.
    m_valPosDbl = new QDoubleValidator(this);
    const double tolerance = 0.00001; // Tolerance chosen arbitrarily.
    m_valPosDbl->setBottom(tolerance);

    // Apply the validator to the input box for the Scale option.
    uiForm().abscor_leScaleMultiplier->setValidator(m_valPosDbl);
  }

  /**
  * Disables/enables the relevant parts of the UI when user checks/unchecks the 'Scale: Multiply Container by' option
  * abscor_ckScaleMultiplier checkbox.
  * @param state :: state of the checkbox
  */
  void ApplyCorr::scaleMultiplierCheck(bool state)
  {
    //scale input should be disabled if we're not using a can
    if(!uiForm().abscor_ckUseCan->isChecked())
    {
      uiForm().abscor_leScaleMultiplier->setEnabled(false);
    }
    else
    {
      //else it should be whatever the scale checkbox is
      state = uiForm().abscor_ckScaleMultiplier->isChecked();
      uiForm().abscor_leScaleMultiplier->setEnabled(state);
    }
  }


  bool ApplyCorr::validateScaleInput()
  {
    bool valid = true;
    int dummyPos = 0;

    // scale multiplier
    QString scaleMultiplierText = uiForm().abscor_leScaleMultiplier->text();
    QValidator::State fieldState = uiForm().abscor_leScaleMultiplier->validator()->validate(scaleMultiplierText, dummyPos);

    if ( uiForm().abscor_ckScaleMultiplier->isChecked() && fieldState != QValidator::Acceptable )
    {
      valid = false;
    }

    return valid;
  }

  void ApplyCorr::run()
  {
    if ( ! validateScaleInput() )
    {
      return;
    }

    QString geom = uiForm().abscor_cbGeometry->currentText();
    if ( geom == "Flat" )
    {
      geom = "flt";
    }
    else if ( geom == "Cylinder" )
    {
      geom = "cyl";
    }

    QString pyInput = "from IndirectDataAnalysis import abscorFeeder, loadNexus\n";

    QString sample = uiForm().abscor_dsSample->getCurrentDataName();
    if (!Mantid::API::AnalysisDataService::Instance().doesExist(sample.toStdString()) )
    {
      pyInput +=
        "sample = loadNexus(r'" + uiForm().abscor_dsSample->getFullFilePath() + "')\n";
    }
    else
    {
      pyInput +=
        "sample = '" + sample + "'\n";
    }

    bool noContainer = false;
    if ( uiForm().abscor_ckUseCan->isChecked() )
    {
      QString container = uiForm().abscor_dsContainer->getCurrentDataName();
      if ( !Mantid::API::AnalysisDataService::Instance().doesExist(container.toStdString()) )
      {
        pyInput +=
          "container = loadNexus(r'" + uiForm().abscor_dsContainer->getFullFilePath() + "')\n";
      }
      else
      {
        pyInput +=
          "container = '" + container + "'\n";
      }
    }
    else
    {
      pyInput += "container = ''\n";
      noContainer = true;
    }

    pyInput += "geom = '" + geom + "'\n";

    
    if( uiForm().abscor_ckUseCorrections->isChecked() )
    {
      pyInput += "useCor = True\n";
      QString corrections = uiForm().abscor_dsCorrections->getCurrentDataName();
      if ( !Mantid::API::AnalysisDataService::Instance().doesExist(corrections.toStdString()) )
      {
        pyInput +=
          "corrections = loadNexus(r'" + uiForm().abscor_dsCorrections->getFullFilePath() + "')\n";
      }
      else
      {
        pyInput +=
          "corrections = '" + corrections + "'\n";
      }
    }
    else
    {
      pyInput += "useCor = False\n";
      pyInput += "corrections = ''\n";

      // if we have no container and no corrections then abort
      if(noContainer)
      {
        showInformationBox("Apply Corrections requires either a can file or a corrections file.");
        return;
      }
    }
    
    QString ScalingFactor = "1.0\n";
    QString ScaleOrNot = "False\n";

    pyInput += uiForm().abscor_ckScaleMultiplier->isChecked() ? "True\n" : "False\n";

    if ( uiForm().abscor_ckScaleMultiplier->isChecked() )
    {
      ScalingFactor = uiForm().abscor_leScaleMultiplier->text();  
      ScaleOrNot = "True\n";
    }

    pyInput += "scale = " + ScaleOrNot + "\n";
    pyInput += "scaleFactor = " + ScalingFactor + "\n";  


    if ( uiForm().abscor_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( uiForm().abscor_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    QString plotResult = uiForm().abscor_cbPlotOutput->currentText();
    if ( plotResult == "Contour" )
    {
      plotResult = "Contour";
    }
    else if ( plotResult == "Spectra" )
    {
      plotResult = "Spectrum";
    }
    else if ( plotResult == "Both" )
    {
      plotResult = "Both";
    }
    
    pyInput += "plotResult = '" + plotResult + "'\n";
        
    if ( uiForm().abscor_ckPlotContrib->isChecked() ) pyInput += "plotContrib = True\n";
    else pyInput += "plotContrib = False\n";

    pyInput += "abscorFeeder(sample, container, geom, useCor, corrections, Verbose=verbose, ScaleOrNotToScale=scale, factor=scaleFactor, Save=save, PlotResult=plotResult, PlotContrib=plotContrib)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }

  QString ApplyCorr::validate()
  {
    return "";
  }

  void ApplyCorr::loadSettings(const QSettings & settings)
  {
    uiForm().abscor_dsCorrections->readSettings(settings.group());
    uiForm().abscor_dsContainer->readSettings(settings.group());
    uiForm().abscor_dsSample->readSettings(settings.group());
  }

  /**
   * Handles when the type of geometry changes
   * 
   * Updates the file extension to search for
   */
  void ApplyCorr::handleGeometryChange(int index)
  {
    QString ext("");
    switch(index)
    {
      case 0:
        // Geomtry is flat
        ext = "_flt_Abs";
        uiForm().abscor_dsCorrections->setWSSuffixes(QStringList(ext));
        uiForm().abscor_dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
        break;
      case 1:
        // Geomtry is cylinder
        ext = "_cyl_Abs";
        uiForm().abscor_dsCorrections->setWSSuffixes(QStringList(ext));
        uiForm().abscor_dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
        break;
    }
  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
