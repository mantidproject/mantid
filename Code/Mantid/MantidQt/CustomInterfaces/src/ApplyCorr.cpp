#include "MantidQtCustomInterfaces/ApplyCorr.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"

#include <QStringList>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("ApplyCorr");
}

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
    connect(uiForm().abscor_dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(newData(const QString&)));
    connect(uiForm().abscor_spPreviewSpec, SIGNAL(valueChanged(int)), this, SLOT(plotPreview(int)));

    // Create a validator for input box of the Scale option.
    const double tolerance = 0.00001; // Tolerance chosen arbitrarily.
    m_valPosDbl->setBottom(tolerance);

    // Apply the validator to the input box for the Scale option.
    uiForm().abscor_leScaleMultiplier->setValidator(m_valPosDbl);

    // Create the plot
    m_plots["ApplyCorrPlot"] = new QwtPlot(m_parentWidget);
    m_plots["ApplyCorrPlot"]->setCanvasBackground(Qt::white);
    m_plots["ApplyCorrPlot"]->setAxisFont(QwtPlot::xBottom, m_parentWidget->font());
    m_plots["ApplyCorrPlot"]->setAxisFont(QwtPlot::yLeft, m_parentWidget->font());
	  uiForm().abscor_plotPreview->addWidget(m_plots["ApplyCorrPlot"]);

    uiForm().abscor_spPreviewSpec->setMinimum(0);
    uiForm().abscor_spPreviewSpec->setMaximum(0);
  }

  /**
  * Disables/enables the relevant parts of the UI when user checks/unchecks the 'Scale: Multiply Container by' option
  * abscor_ckScaleMultiplier checkbox.
  * @param state :: state of the checkbox
  */
  void ApplyCorr::scaleMultiplierCheck(bool state)
  {
    // Scale input should be disabled if we're not using a can
    if(!uiForm().abscor_ckUseCan->isChecked())
    {
      uiForm().abscor_leScaleMultiplier->setEnabled(false);
    }
    else
    {
      // Else it should be whatever the scale checkbox is
      state = uiForm().abscor_ckScaleMultiplier->isChecked();
      uiForm().abscor_leScaleMultiplier->setEnabled(state);
    }
  }

  /**
   * Disables corrections when using S(Q, w) as input data.
   *
   * @param dataName Name of new data source
   */
  void ApplyCorr::newData(const QString &dataName)
  {
    removeCurve("CalcCurve");
    removeCurve("CanCurve");
    // removeCurve would usually need a replot() but this is done in plotMiniPlot()

    plotMiniPlot(dataName, 0, "ApplyCorrPlot", "ApplyCorrSampleCurve");

    MatrixWorkspace_const_sptr sampleWs =  AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(dataName.toStdString());
    uiForm().abscor_spPreviewSpec->setMaximum(static_cast<int>(sampleWs->getNumberHistograms()) - 1);
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
    MatrixWorkspace_const_sptr sampleWs =  AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(sample.toStdString());

    pyInput += "sample = '"+sample+"'\n";
    pyInput += "rebin_can = False\n";
    bool noContainer = false;

    bool useCan = uiForm().abscor_ckUseCan->isChecked();
    if(useCan)
    {
      QString container = uiForm().abscor_dsContainer->getCurrentDataName();
      MatrixWorkspace_const_sptr canWs =  AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(container.toStdString());

      if (!checkWorkspaceBinningMatches(sampleWs, canWs))
      {
        if (requireCanRebin())
        {
          pyInput += "rebin_can = True\n";
        }
        else
        {
          //user clicked cancel and didn't want to rebin, so just do nothing.
          return;
        }
      }

      pyInput += "container = '" + container + "'\n";
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
        showMessageBox("Apply Corrections requires either a can file or a corrections file.");
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
    pyInput += "print abscorFeeder(sample, container, geom, useCor, corrections, Verbose=verbose, RebinCan=rebin_can, ScaleOrNotToScale=scale, factor=scaleFactor, Save=save, PlotResult=plotResult)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();

    outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(pyOutput.toStdString());
    plotPreview(uiForm().abscor_spPreviewSpec->value());

    // Set the result workspace for Python script export
    m_pythonExportWsName = pyOutput.toStdString();
  }

  /**
   * Ask the user is they wish to rebin the can to the sample.
   * @return whether a rebin of the can workspace is required.
   */
  bool ApplyCorr::requireCanRebin()
  {
    QString message = "The sample and can energy ranges do not match, this is not recommended."
        "\n\n Click OK to rebin the can to match the sample and continue or Cancel to abort applying corrections.";
    QMessageBox::StandardButton reply = QMessageBox::warning(m_parentWidget, "Energy Ranges Do Not Match", 
                                                             message, QMessageBox::Ok|QMessageBox::Cancel);
    return (reply == QMessageBox::Ok);
  }

  bool ApplyCorr::validate()
  {
    bool useCan = uiForm().abscor_ckUseCan->isChecked();

    if(useCan)
    {
      QString sample = uiForm().abscor_dsSample->getCurrentDataName();
      QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
      QString container = uiForm().abscor_dsContainer->getCurrentDataName();
      QString containerType = container.right(container.length() - container.lastIndexOf("_"));

      g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
      g_log.debug() << "Container type is: " << containerType.toStdString() << std::endl;

      if(containerType != sampleType)
      {
        g_log.error("Must use the same type of files for sample and container inputs.");
        return false;
      }
    }

    return true;
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
        ext = "_flt_abs";
        uiForm().abscor_dsCorrections->setWSSuffixes(QStringList(ext));
        uiForm().abscor_dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
        break;
      case 1:
        // Geomtry is cylinder
        ext = "_cyl_abs";
        uiForm().abscor_dsCorrections->setWSSuffixes(QStringList(ext));
        uiForm().abscor_dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
        break;
    }
  }
  
  /**
   * Replots the preview plot.
   *
   * @param specIndex Spectrum index to plot
   */
  void ApplyCorr::plotPreview(int specIndex)
  {
    bool useCan = uiForm().abscor_ckUseCan->isChecked();

    // Plot sample
    QString sample = uiForm().abscor_dsSample->getCurrentDataName();
    if(AnalysisDataService::Instance().doesExist(sample.toStdString()))
    {
      MatrixWorkspace_const_sptr sampleWs =  AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(sample.toStdString());
      plotMiniPlot(sampleWs, specIndex, "ApplyCorrPlot", "ApplyCorrSampleCurve");
    }

    // Plot result
    if(outputWs)
    {
      plotMiniPlot(outputWs, specIndex, "ApplyCorrPlot", "CalcCurve");
      m_curves["CalcCurve"]->setPen(QColor(Qt::green));
    }

    // Plot can
    if(useCan)
    {
      QString container = uiForm().abscor_dsContainer->getCurrentDataName();
      MatrixWorkspace_const_sptr canWs =  AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(container.toStdString());
      plotMiniPlot(canWs, specIndex, "ApplyCorrPlot", "CanCurve");
      m_curves["CanCurve"]->setPen(QColor(Qt::red));
    }
    else
      removeCurve("CanCurve");

    replot("ApplyCorrPlot");
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
