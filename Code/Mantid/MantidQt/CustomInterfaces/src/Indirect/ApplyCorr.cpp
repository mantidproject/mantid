#include "MantidQtCustomInterfaces/Indirect/ApplyCorr.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"

#include <QStringList>


using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("ApplyCorr");
}

//TODO: Preview plot is not happy when an input workspace is not in wavelength

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

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
    connect(m_uiForm.cbGeometry, SIGNAL(currentIndexChanged(int)), this, SLOT(handleGeometryChange(int)));
    connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(newData(const QString&)));
    connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this, SLOT(plotPreview(int)));

    m_uiForm.spPreviewSpec->setMinimum(0);
    m_uiForm.spPreviewSpec->setMaximum(0);
  }


  void ApplyCorr::setup()
  {
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
    API::BatchAlgorithmRunner::AlgorithmRuntimeProps absCorProps;
    IAlgorithm_sptr applyCorrAlg = AlgorithmManager::Instance().create("ApplyPaalmanPingsCorrection");
    applyCorrAlg->initialize();

    QString sampleWsName = m_uiForm.dsSample->getCurrentDataName();
    MatrixWorkspace_sptr sampleWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sampleWsName.toStdString());
    Mantid::Kernel::Unit_sptr sampleXUnit = sampleWs->getAxis(0)->unit();

    // If not in wavelength then do conversion
    if(sampleXUnit->caption() != "Wavelength")
    {
      g_log.information("Sample workspace not in wavelength, need to convert to continue.");
      absCorProps["SampleWorkspace"] = addUnitConversionStep(sampleWs);
    }
    else
    {
      absCorProps["SampleWorkspace"] = sampleWsName.toStdString();
    }

    bool useCan = m_uiForm.ckUseCan->isChecked();
    if(useCan)
    {
      QString canWsName = m_uiForm.dsContainer->getCurrentDataName();
      MatrixWorkspace_sptr canWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(canWsName.toStdString());

      // If not in wavelength then do conversion
      Mantid::Kernel::Unit_sptr canXUnit = canWs->getAxis(0)->unit();
      if(canXUnit->caption() != "Wavelength")
      {
        g_log.information("Container workspace not in wavelength, need to convert to continue.");
        absCorProps["CanWorkspace"] = addUnitConversionStep(canWs);
      }
      else
      {
        absCorProps["CanWorkspace"] = canWsName.toStdString();
      }

      bool useCanScale = m_uiForm.ckScaleCan->isChecked();
      if(useCanScale)
      {
        double canScaleFactor = m_uiForm.spCanScale->value();
        applyCorrAlg->setProperty("CanScaleFactor", canScaleFactor);
      }

      // Check for same binning across sample and container
      if(!checkWorkspaceBinningMatches(sampleWs, canWs))
      {
        QString text = "Binning on sample and container does not match."
                       "Would you like to rebin the sample to match the container?";

        int result = QMessageBox::question(NULL, tr("Rebin sample?"), tr(text),
                                           QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if(result == QMessageBox::Yes)
        {
          addRebinStep(sampleWsName, canWsName);
        }
        else
        {
          m_batchAlgoRunner->clearQueue();
          g_log.error("Cannot apply absorption corrections using a sample and container with different binning.");
          return;
        }
      }
    }

    bool useCorrections = m_uiForm.ckUseCorrections->isChecked();
    if(useCorrections)
    {
      QString correctionsWsName = m_uiForm.dsCorrections->getCurrentDataName();

      WorkspaceGroup_sptr corrections = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(correctionsWsName.toStdString());
      bool interpolateAll = false;
      for(size_t i = 0; i < corrections->size(); i++)
      {
        MatrixWorkspace_sptr factorWs = boost::dynamic_pointer_cast<MatrixWorkspace>(corrections->getItem(i));

        // Check for matching binning
        if(sampleWs && (sampleWs->blocksize() != factorWs->blocksize()))
        {
          int result;
          if(interpolateAll)
          {
            result = QMessageBox::Yes;
          }
          else
          {
            QString text = "Number of bins on sample and "
                         + QString::fromStdString(factorWs->name())
                         + " workspace does not match.\n"
                         + "Would you like to interpolate this workspace to match the sample?";

            result = QMessageBox::question(NULL, tr("Interpolate corrections?"), tr(text),
                                           QMessageBox::YesToAll, QMessageBox::Yes, QMessageBox::No);
          }

          switch(result)
          {
            case QMessageBox::YesToAll:
              interpolateAll = true;
            case QMessageBox::Yes:
              addInterpolationStep(factorWs, absCorProps["SampleWorkspace"]);
              break;
            default:
              m_batchAlgoRunner->clearQueue();
              g_log.error("ApplyCorr cannot run with corrections that do not match sample binning.");
              return;
          }
        }
      }

      applyCorrAlg->setProperty("CorrectionsWorkspace", correctionsWsName.toStdString());
    }

    // Generate output workspace name
    int nameCutIndex = sampleWsName.lastIndexOf("_");
    if(nameCutIndex == -1)
      nameCutIndex = sampleWsName.length();

    QString correctionType;
    switch(m_uiForm.cbGeometry->currentIndex())
    {
      case 0:
        correctionType = "flt";
        break;
      case 1:
        correctionType = "cyl";
        break;
    }
    QString outputWsName = sampleWsName.left(nameCutIndex) + + "_" + correctionType + "_Corrected";

    applyCorrAlg->setProperty("OutputWorkspace", outputWsName.toStdString());

    // Run the corrections algorithm
    m_batchAlgoRunner->addAlgorithm(applyCorrAlg, absCorProps);
    m_batchAlgoRunner->executeBatchAsync();

    // Set the result workspace for Python script export
    m_pythonExportWsName = outputWsName.toStdString();
  }


  /**
   * Adds a unit converstion step to the batch algorithm queue.
   *
   * @param ws Pointer to the workspace to convert
   * @return Name of output workspace
   */
  std::string ApplyCorr::addUnitConversionStep(MatrixWorkspace_sptr ws)
  {
    std::string outputName = ws->name() + "_inWavelength";

    IAlgorithm_sptr convertAlg = AlgorithmManager::Instance().create("ConvertUnits");
    convertAlg->initialize();

    convertAlg->setProperty("InputWorkspace", ws->name());
    convertAlg->setProperty("OutputWorkspace", outputName);
    convertAlg->setProperty("Target", "Wavelength");
    convertAlg->setProperty("EMode", getEMode(ws));
    convertAlg->setProperty("EFixed", getEFixed(ws));

    m_batchAlgoRunner->addAlgorithm(convertAlg);

    return outputName;
  }


  /**
   * Adds a rebin to workspace step to the calculation for when using a sample and container that
   * have different binning.
   *
   * @param toRebin
   * @param toMatch
   */
  void ApplyCorr::addRebinStep(QString toRebin, QString toMatch)
  {
    API::BatchAlgorithmRunner::AlgorithmRuntimeProps rebinProps;
    rebinProps["WorkspaceToMatch"] = toMatch.toStdString();

    IAlgorithm_sptr rebinAlg = AlgorithmManager::Instance().create("RebinToWorkspace");
    rebinAlg->initialize();

    rebinAlg->setProperty("WorkspaceToRebin", toRebin.toStdString());
    rebinAlg->setProperty("OutputWorkspace", toRebin.toStdString());

    m_batchAlgoRunner->addAlgorithm(rebinAlg, rebinProps);
  }


  /**
   * Adds a spline interpolation as a step in the calculation for using legacy correction factor
   * workspaces.
   *
   * @param toInterpolate Pointer to the workspace to interpolate
   * @param toMatch Name of the workspace to match
   */
  void ApplyCorr::addInterpolationStep(MatrixWorkspace_sptr toInterpolate, std::string toMatch)
  {
    API::BatchAlgorithmRunner::AlgorithmRuntimeProps interpolationProps;
    interpolationProps["WorkspaceToMatch"] = toMatch;

    IAlgorithm_sptr interpolationAlg = AlgorithmManager::Instance().create("SplineInterpolation");
    interpolationAlg->initialize();

    interpolationAlg->setProperty("WorkspaceToInterpolate", toInterpolate->name());
    interpolationAlg->setProperty("OutputWorkspace", toInterpolate->name());

    m_batchAlgoRunner->addAlgorithm(interpolationAlg, interpolationProps);
  }


  /**
   * Handles completion of the algorithm.
   *
   * @param error True if algorithm failed.
   */
  void ApplyCorr::algorithmComplete(bool error)
  {
    if(error)
    {
      emit showMessageBox("Unable to apply corrections.\nSee Results Log for more details.");
      return;
    }

    m_outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_pythonExportWsName);
    plotPreview(m_uiForm.spPreviewSpec->value());
  }


  bool ApplyCorr::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

    MatrixWorkspace_sptr sampleWs;

    bool useCan = m_uiForm.ckUseCan->isChecked();
    bool useCorrections = m_uiForm.ckUseCorrections->isChecked();

    if(!(useCan || useCorrections))
      uiv.addErrorMessage("Must use either container subtraction or corrections");

    if(useCan)
    {
      uiv.checkDataSelectorIsValid("Container", m_uiForm.dsContainer);

      // Check can and sample workspaces are the same "type" (reduced or S(Q, w))
      QString sample = m_uiForm.dsSample->getCurrentDataName();
      QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
      QString container = m_uiForm.dsContainer->getCurrentDataName();
      QString containerType = container.right(container.length() - container.lastIndexOf("_"));

      g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
      g_log.debug() << "Can type is: " << containerType.toStdString() << std::endl;

      if(containerType != sampleType)
        uiv.addErrorMessage("Sample and can workspaces must contain the same type of data.");
    }

    if(useCorrections)
    {
      uiv.checkDataSelectorIsValid("Corrections", m_uiForm.dsCorrections);

      QString correctionsWsName = m_uiForm.dsCorrections->getCurrentDataName();
      WorkspaceGroup_sptr corrections = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(correctionsWsName.toStdString());
      for(size_t i = 0; i < corrections->size(); i++)
      {
        // Check it is a MatrixWorkspace
        MatrixWorkspace_sptr factorWs = boost::dynamic_pointer_cast<MatrixWorkspace>(corrections->getItem(i));
        if(!factorWs)
        {
          QString msg = "Correction factor workspace "
                      + QString::number(i)
                      + " is not a MatrixWorkspace";
          uiv.addErrorMessage(msg);
          continue;
        }

        // Check X unit is wavelength
        Mantid::Kernel::Unit_sptr xUnit = factorWs->getAxis(0)->unit();
        if(xUnit->caption() != "Wavelength")
        {
          QString msg = "Correction factor workspace "
                      + QString::fromStdString(factorWs->name())
                      + " is not in wavelength";
          uiv.addErrorMessage(msg);
        }
      }
    }

    // Show errors if there are any
    if(!uiv.isAllInputValid())
      emit showMessageBox(uiv.generateErrorMessage());

    return uiv.isAllInputValid();
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
