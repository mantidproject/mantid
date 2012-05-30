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

    pyInput += "abscorFeeder(sample, container, geom, useCor)\n";
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
