#include "MantidQtCustomInterfaces/ApplyCorr.h"

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
    // Disable Container inputs is "Use Container" is not checked
    connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_lbContainerInputType, SLOT(setEnabled(bool)));
    connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_cbContainerInputType, SLOT(setEnabled(bool)));
    connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_swContainerInput, SLOT(setEnabled(bool)));

    connect(uiForm().abscor_cbSampleInputType, SIGNAL(currentIndexChanged(int)), uiForm().abscor_swSampleInput, SLOT(setCurrentIndex(int)));
    connect(uiForm().abscor_cbContainerInputType, SIGNAL(currentIndexChanged(int)), uiForm().abscor_swContainerInput, SLOT(setCurrentIndex(int)));
    
    connect(uiForm().abscor_ckScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(scaleMultiplierCheck(bool)));

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
    uiForm().abscor_leScaleMultiplier->setEnabled(state);
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

    if ( uiForm().abscor_cbSampleInputType->currentText() == "File" )
    {
      pyInput +=
        "sample = loadNexus(r'" + uiForm().abscor_sample->getFirstFilename() + "')\n";
    }
    else
    {
      pyInput +=
        "sample = '" + uiForm().abscor_wsSample->currentText() + "'\n";
    }

    if ( uiForm().abscor_ckUseCan->isChecked() )
    {
      if ( uiForm().abscor_cbContainerInputType->currentText() == "File" )
      {
        pyInput +=
          "container = loadNexus(r'" + uiForm().abscor_can->getFirstFilename() + "')\n";
      }
      else
      {
        pyInput +=
          "container = '" + uiForm().abscor_wsContainer->currentText() + "'\n";
      }
    }
    else
    {
      pyInput += "container = ''\n";
    }

    pyInput += "geom = '" + geom + "'\n";


    if ( uiForm().abscor_ckUseCorrections->isChecked() )
    {
      pyInput += "useCor = True\n";
    }
    else
    {
      pyInput += "useCor = False\n";
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

    pyInput += "abscorFeeder(sample, container, geom, useCor, Verbose=verbose, ScaleOrNotToScale=scale, factor=scaleFactor, Save=save, PlotResult=plotResult, PlotContrib=plotContrib)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }

  QString ApplyCorr::validate()
  {
    return "";
  }

  void ApplyCorr::loadSettings(const QSettings & settings)
  {
    uiForm().abscor_sample->readSettings(settings.group());
    uiForm().abscor_can->readSettings(settings.group());
  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
