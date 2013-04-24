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
  }

  void ApplyCorr::run()
  {
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

    pyInput += "abscorFeeder(sample, container, geom, useCor, Verbose=verbose, Scale=False, factor=1, Save=save, PlotResult=plotResult, PlotContrib=plotContrib)\n";

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
