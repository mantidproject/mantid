#include "MantidQtCustomInterfaces/Indirect/ApplyCorr.h"
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
    m_uiForm.setupUi(parent);
  }

  void ApplyCorr::setup()
  {
    connect(m_uiForm.cbGeometry, SIGNAL(currentIndexChanged(int)), this, SLOT(handleGeometryChange(int)));
    connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(newData(const QString&)));
    connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this, SLOT(plotPreview(int)));

    m_uiForm.spPreviewSpec->setMinimum(0);
    m_uiForm.spPreviewSpec->setMaximum(0);
  }

  /**
   * Disables corrections when using S(Q, w) as input data.
   *
   * @param dataName Name of new data source
   */
  void ApplyCorr::newData(const QString &dataName)
  {
    const MatrixWorkspace_sptr sampleWs =  AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(dataName.toStdString());
    m_uiForm.spPreviewSpec->setMaximum(static_cast<int>(sampleWs->getNumberHistograms()) - 1);

    // Plot the sample curve
    m_uiForm.ppPreview->clear();
    m_uiForm.ppPreview->addSpectrum("Sample", sampleWs, 0, Qt::black);
  }

  void ApplyCorr::run()
  {
    QString geom = m_uiForm.cbGeometry->currentText();
    if ( geom == "Flat" )
    {
      geom = "flt";
    }
    else if ( geom == "Cylinder" )
    {
      geom = "cyl";
    }

    QString pyInput = "from IndirectDataAnalysis import abscorFeeder, loadNexus\n";

    QString sample = m_uiForm.dsSample->getCurrentDataName();
    MatrixWorkspace_const_sptr sampleWs =  AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(sample.toStdString());

    pyInput += "sample = '"+sample+"'\n";
    pyInput += "rebin_can = False\n";
    bool noContainer = false;

    bool useCan = m_uiForm.ckUseCan->isChecked();
    if(useCan)
    {
      QString container = m_uiForm.dsContainer->getCurrentDataName();
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

    if( m_uiForm.ckUseCorrections->isChecked() )
    {
      pyInput += "useCor = True\n";
      QString corrections = m_uiForm.dsCorrections->getCurrentDataName();
      if ( !Mantid::API::AnalysisDataService::Instance().doesExist(corrections.toStdString()) )
      {
        pyInput +=
          "corrections = loadNexus(r'" + m_uiForm.dsCorrections->getFullFilePath() + "')\n";
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

    pyInput += m_uiForm.ckScaleCan->isChecked() ? "True\n" : "False\n";

    if ( m_uiForm.ckScaleCan->isChecked() )
    {
      ScalingFactor = m_uiForm.spCanScale->text();
      ScaleOrNot = "True\n";
    }

    pyInput += "scale = " + ScaleOrNot + "\n";
    pyInput += "scaleFactor = " + ScalingFactor + "\n";

    if ( m_uiForm.ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    QString plotResult = m_uiForm.cbPlotOutput->currentText();
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
    pyInput += "print abscorFeeder(sample, container, geom, useCor, corrections, RebinCan=rebin_can, ScaleOrNotToScale=scale, factor=scaleFactor, Save=save, PlotResult=plotResult)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();

    m_outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(pyOutput.toStdString());
    plotPreview(m_uiForm.spPreviewSpec->value());

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
    bool useCan = m_uiForm.ckUseCan->isChecked();

    if(useCan)
    {
      QString sample = m_uiForm.dsSample->getCurrentDataName();
      QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
      QString container = m_uiForm.dsContainer->getCurrentDataName();
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
    m_uiForm.dsCorrections->readSettings(settings.group());
    m_uiForm.dsContainer->readSettings(settings.group());
    m_uiForm.dsSample->readSettings(settings.group());
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
        m_uiForm.dsCorrections->setWSSuffixes(QStringList(ext));
        m_uiForm.dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
        break;
      case 1:
        // Geomtry is cylinder
        ext = "_cyl_abs";
        m_uiForm.dsCorrections->setWSSuffixes(QStringList(ext));
        m_uiForm.dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
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
    bool useCan = m_uiForm.ckUseCan->isChecked();

    m_uiForm.ppPreview->clear();

    // Plot sample
    const QString sample = m_uiForm.dsSample->getCurrentDataName();
    if(AnalysisDataService::Instance().doesExist(sample.toStdString()))
    {
      m_uiForm.ppPreview->addSpectrum("Sample", sample, specIndex, Qt::black);
    }

    // Plot result
    if(m_outputWs)
    {
      m_uiForm.ppPreview->addSpectrum("Corrected", m_outputWs, specIndex, Qt::green);
    }

    // Plot can
    if(useCan)
    {
      QString container = m_uiForm.dsContainer->getCurrentDataName();
      const MatrixWorkspace_sptr canWs =  AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(container.toStdString());
      m_uiForm.ppPreview->addSpectrum("Can", canWs, specIndex, Qt::red);
    }
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
