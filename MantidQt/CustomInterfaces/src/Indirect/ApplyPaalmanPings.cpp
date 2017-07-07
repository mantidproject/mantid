#include "MantidQtCustomInterfaces/Indirect/ApplyPaalmanPings.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QStringList>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ApplyPaalmanPings");
}

namespace MantidQt {
namespace CustomInterfaces {
ApplyPaalmanPings::ApplyPaalmanPings(QWidget *parent) : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.cbGeometry, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleGeometryChange(int)));
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(newSample(const QString &)));
  connect(m_uiForm.dsContainer, SIGNAL(dataReady(const QString &)), this,
          SLOT(newContainer(const QString &)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotPreview(int)));
  connect(m_uiForm.spCanScale, SIGNAL(valueChanged(double)), this,
          SLOT(updateContainer()));
  connect(m_uiForm.spCanShift, SIGNAL(valueChanged(double)), this,
          SLOT(updateContainer()));
  connect(m_uiForm.ckShiftCan, SIGNAL(toggled(bool)), this,
          SLOT(updateContainer()));
  connect(m_uiForm.ckScaleCan, SIGNAL(toggled(bool)), this,
          SLOT(updateContainer()));
  connect(m_uiForm.ckRebinContainer, SIGNAL(toggled(bool)), this,
          SLOT(updateContainer()));
  connect(m_uiForm.ckUseCan, SIGNAL(toggled(bool)), this,
          SLOT(updateContainer()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

void ApplyPaalmanPings::setup() {}

/**
* Disables corrections when using S(Q, w) as input data.
*
* @param dataName Name of new data source
*/
void ApplyPaalmanPings::newSample(const QString &dataName) {
  // Remove old curves
  m_uiForm.ppPreview->removeSpectrum("Sample");
  m_uiForm.ppPreview->removeSpectrum("Corrected");

  // Get workspace
  const MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          dataName.toStdString());

  // Plot the curve
  m_uiForm.ppPreview->addSpectrum("Sample", sampleWs, 0, Qt::black);
  m_uiForm.spPreviewSpec->setMaximum(
      static_cast<int>(sampleWs->getNumberHistograms()) - 1);
  m_sampleWorkspaceName = dataName.toStdString();

  // Set maximum / minimum can shift
  m_uiForm.spCanShift->setMinimum(sampleWs->getXMin());
  m_uiForm.spCanShift->setMaximum(sampleWs->getXMax());
}

void ApplyPaalmanPings::newContainer(const QString &dataName) {
  // Remove old curves
  m_uiForm.ppPreview->removeSpectrum("Container");
  m_uiForm.ppPreview->removeSpectrum("Corrected");

  // get Workspace
  const MatrixWorkspace_sptr containerWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          dataName.toStdString());
  // Clone for use in plotting and alg
  IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
  clone->initialize();
  clone->setProperty("InputWorkspace", containerWs);
  clone->setProperty("Outputworkspace", "__processed_can");
  clone->execute();
  m_uiForm.ppPreview->addSpectrum("Container", containerWs, 0, Qt::red);
  m_containerWorkspaceName = "__processed_can";
}

void ApplyPaalmanPings::updateContainer() {
  const auto canName = m_uiForm.dsContainer->getCurrentDataName();
  const auto canValid = m_uiForm.dsContainer->isValid();
  const auto useCan = m_uiForm.ckUseCan->isChecked();
  if (canValid && useCan) {
    auto shift = m_uiForm.spCanShift->value();
    if (!m_uiForm.ckShiftCan->isChecked())
      shift = 0.0;

    auto scale = m_uiForm.spCanScale->value();
    if (!m_uiForm.ckScaleCan->isChecked())
      scale = 1.0;

    IAlgorithm_sptr scaleXAlg = AlgorithmManager::Instance().create("ScaleX");
    scaleXAlg->initialize();
    scaleXAlg->setLogging(false);
    scaleXAlg->setProperty("InputWorkspace", canName.toStdString());
    scaleXAlg->setProperty("OutputWorkspace", m_containerWorkspaceName);
    scaleXAlg->setProperty("Factor", shift);
    scaleXAlg->setProperty("Operation", "Add");
    scaleXAlg->execute();

    IAlgorithm_sptr scaleAlg = AlgorithmManager::Instance().create("Scale");
    scaleAlg->initialize();
    scaleAlg->setLogging(false);
    scaleAlg->setProperty("InputWorkspace", m_containerWorkspaceName);
    scaleAlg->setProperty("OutputWorkspace", m_containerWorkspaceName);
    scaleAlg->setProperty("Factor", scale);
    scaleAlg->setProperty("Operation", "Multiply");
    scaleAlg->execute();

    const auto sampleValid = m_uiForm.dsSample->isValid();
    if (sampleValid && m_uiForm.ckRebinContainer->isChecked()) {
      IAlgorithm_sptr rebin =
          AlgorithmManager::Instance().create("RebinToWorkspace");
      rebin->initialize();
      rebin->setLogging(false);
      rebin->setProperty("WorkspaceToRebin", m_containerWorkspaceName);
      rebin->setProperty("WorkspaceToMatch", m_sampleWorkspaceName);
      rebin->setProperty("OutputWorkspace", m_containerWorkspaceName);
      rebin->execute();
    } else if (!sampleValid) {
      // Sample was not valid so do not rebin
      m_uiForm.ppPreview->removeSpectrum("Container");
      return;
    }
  } else {
    // Can was not valid so do not replot
    m_uiForm.ppPreview->removeSpectrum("Container");
    return;
  }
  plotPreview(m_uiForm.spPreviewSpec->value());
}

void ApplyPaalmanPings::run() {
  // Create / Initialize algorithm
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps absCorProps;
  IAlgorithm_sptr applyCorrAlg =
      AlgorithmManager::Instance().create("ApplyPaalmanPingsCorrection");
  applyCorrAlg->initialize();

  // get Sample Workspace
  MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_sampleWorkspaceName);
  absCorProps["SampleWorkspace"] = m_sampleWorkspaceName;

  const bool useCan = m_uiForm.ckUseCan->isChecked();
  const bool useCorrections = m_uiForm.ckUseCorrections->isChecked();
  // Get Can and Clone
  MatrixWorkspace_sptr canClone;
  if (useCan) {
    const auto canName =
        m_uiForm.dsContainer->getCurrentDataName().toStdString();
    const auto cloneName = "__algorithm_can";
    IAlgorithm_sptr clone =
        AlgorithmManager::Instance().create("CloneWorkspace");
    clone->initialize();
    clone->setProperty("InputWorkspace", canName);
    clone->setProperty("Outputworkspace", cloneName);
    clone->execute();

    canClone =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(cloneName);
    // Check for same binning across sample and container
    if (!checkWorkspaceBinningMatches(sampleWs, canClone)) {
      const char *text = "Binning on sample and container does not match."
                         "Would you like to enable rebinning of the container?";

      int result = QMessageBox::question(NULL, tr("Rebin sample?"), tr(text),
                                         QMessageBox::Yes, QMessageBox::No,
                                         QMessageBox::NoButton);

      if (result == QMessageBox::Yes) {
        m_uiForm.ckRebinContainer->setChecked(true);
      } else {
        m_batchAlgoRunner->clearQueue();
        g_log.error("Cannot apply absorption corrections "
                    "using a sample and "
                    "container with different binning.");
        return;
      }
    }

    absCorProps["CanWorkspace"] = cloneName;

    const bool useCanScale = m_uiForm.ckScaleCan->isChecked();
    if (useCanScale) {
      const double canScaleFactor = m_uiForm.spCanScale->value();
      applyCorrAlg->setProperty("CanScaleFactor", canScaleFactor);
    }
    if (m_uiForm.ckShiftCan->isChecked()) { // If container is shifted
      const double canShiftFactor = m_uiForm.spCanShift->value();
      applyCorrAlg->setProperty("canShiftFactor", canShiftFactor);
    }
    const bool rebinContainer = m_uiForm.ckRebinContainer->isChecked();
    applyCorrAlg->setProperty("RebinCanToSample", rebinContainer);
  }

  if (useCorrections) {
    QString correctionsWsName = m_uiForm.dsCorrections->getCurrentDataName();

    WorkspaceGroup_sptr corrections =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            correctionsWsName.toStdString());
    bool interpolateAll = false;
    for (size_t i = 0; i < corrections->size(); i++) {
      MatrixWorkspace_sptr factorWs =
          boost::dynamic_pointer_cast<MatrixWorkspace>(corrections->getItem(i));

      // Check for matching binning
      if (sampleWs && (factorWs->blocksize() != sampleWs->blocksize() &&
                       factorWs->blocksize() != 1)) {
        int result;
        if (interpolateAll) {
          result = QMessageBox::Yes;
        } else {
          std::string text = "Number of bins on sample and " +
                             factorWs->getName() +
                             " workspace does not match.\n" +
                             "Would you like to interpolate this workspace to "
                             "match the sample?";

          result = QMessageBox::question(
              NULL, tr("Interpolate corrections?"), tr(text.c_str()),
              QMessageBox::YesToAll, QMessageBox::Yes, QMessageBox::No);
        }

        switch (result) {
        case QMessageBox::YesToAll:
          interpolateAll = true;
        // fall through
        case QMessageBox::Yes:
          addInterpolationStep(factorWs, absCorProps["SampleWorkspace"]);
          break;
        default:
          m_batchAlgoRunner->clearQueue();
          g_log.error("ApplyPaalmanPings cannot run with corrections that do "
                      "not match sample binning.");
          return;
        }
      }
    }

    applyCorrAlg->setProperty("CorrectionsWorkspace",
                              correctionsWsName.toStdString());
  }

  // Generate output workspace name
  auto QStrSampleWsName = QString::fromStdString(m_sampleWorkspaceName);
  int nameCutIndex = QStrSampleWsName.lastIndexOf("_");
  if (nameCutIndex == -1)
    nameCutIndex = QStrSampleWsName.length();

  QString correctionType;
  switch (m_uiForm.cbGeometry->currentIndex()) {
  case 0:
    correctionType = "flt";
    break;
  case 1:
    correctionType = "cyl";
    break;
  case 2:
    correctionType = "anl";
    break;
  }
  QString outputWsName = QStrSampleWsName.left(nameCutIndex);

  // Using corrections
  if (m_uiForm.ckUseCorrections->isChecked()) {
    outputWsName += "_" + correctionType + "_Corrected";
  } else {
    outputWsName += "_Subtracted";
  }

  // Using container
  if (m_uiForm.ckUseCan->isChecked()) {
    const auto canName =
        m_uiForm.dsContainer->getCurrentDataName().toStdString();
    MatrixWorkspace_sptr containerWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(canName);
    auto run = containerWs->run();
    if (run.hasProperty("run_number")) {
      outputWsName +=
          "_" + QString::fromStdString(run.getProperty("run_number")->value());
    } else {
      auto canCutIndex = QString::fromStdString(canName).indexOf("_");
      outputWsName += "_" + QString::fromStdString(canName).left(canCutIndex);
    }
  }

  outputWsName += "_red";

  applyCorrAlg->setProperty("OutputWorkspace", outputWsName.toStdString());

  // Add corrections algorithm to queue
  m_batchAlgoRunner->addAlgorithm(applyCorrAlg, absCorProps);

  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(absCorComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = outputWsName.toStdString();
  // m_containerWorkspaceName = m_uiForm.dsContainer->getCurrentDataName();
  // updateContainer();
}

/**
* Adds a spline interpolation as a step in the calculation for using legacy
*correction factor
* workspaces.
*
* @param toInterpolate Pointer to the workspace to interpolate
* @param toMatch Name of the workspace to match
*/
void ApplyPaalmanPings::addInterpolationStep(MatrixWorkspace_sptr toInterpolate,
                                             std::string toMatch) {
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps interpolationProps;
  interpolationProps["WorkspaceToMatch"] = toMatch;

  IAlgorithm_sptr interpolationAlg =
      AlgorithmManager::Instance().create("SplineInterpolation");
  interpolationAlg->initialize();

  interpolationAlg->setProperty("WorkspaceToInterpolate",
                                toInterpolate->getName());
  interpolationAlg->setProperty("OutputWorkspace", toInterpolate->getName());

  m_batchAlgoRunner->addAlgorithm(interpolationAlg, interpolationProps);
}

/**
* Handles completion of the abs. correction algorithm.
*
* @param error True if algorithm failed.
*/
void ApplyPaalmanPings::absCorComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(absCorComplete(bool)));
  if (error) {
    emit showMessageBox(
        "Unable to apply corrections.\nSee Results Log for more details.");
    return;
  }

  if (m_uiForm.ckUseCan->isChecked()) {
    if (m_uiForm.ckShiftCan->isChecked()) { // If container is shifted
      IAlgorithm_sptr shiftLog =
          AlgorithmManager::Instance().create("AddSampleLog");
      shiftLog->initialize();

      shiftLog->setProperty("Workspace", m_pythonExportWsName);
      shiftLog->setProperty("LogName", "container_shift");
      shiftLog->setProperty("LogType", "Number");
      shiftLog->setProperty("LogText", boost::lexical_cast<std::string>(
                                           m_uiForm.spCanShift->value()));
      m_batchAlgoRunner->addAlgorithm(shiftLog);
    }
  }
  // Run algorithm queue
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(postProcessComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
* Handles completion of the unit conversion and saving algorithm.
*
* @param error True if algorithm failed.
*/
void ApplyPaalmanPings::postProcessComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(postProcessComplete(bool)));

  if (error) {
    emit showMessageBox("Unable to process corrected workspace.\nSee Results "
                        "Log for more details.");
    return;
  }

  // Enable post processing plot and save
  m_uiForm.cbPlotOutput->setEnabled(true);
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);

  // Handle preview plot
  plotPreview(m_uiForm.spPreviewSpec->value());

  // Clean up unwanted workspaces
  IAlgorithm_sptr deleteAlg =
      AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteAlg->initialize();
  deleteAlg->setProperty("Workspace", "__algorithm_can");
  deleteAlg->execute();
  const auto conv =
      AnalysisDataService::Instance().doesExist("__algorithm_can_Wavelength");
  if (conv) {
    deleteAlg->setProperty("Workspace", "__algorithm_can_Wavelength");
    deleteAlg->execute();
  }
}

bool ApplyPaalmanPings::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

  MatrixWorkspace_sptr sampleWs;

  bool useCan = m_uiForm.ckUseCan->isChecked();
  bool useCorrections = m_uiForm.ckUseCorrections->isChecked();

  if (!(useCan || useCorrections))
    uiv.addErrorMessage("Must use either container subtraction or corrections");

  if (useCan) {
    uiv.checkDataSelectorIsValid("Container", m_uiForm.dsContainer);

    // Check can and sample workspaces are the same "type" (reduced or S(Q, w))
    QString sample = m_uiForm.dsSample->getCurrentDataName();
    QString sampleType =
        sample.right(sample.length() - sample.lastIndexOf("_"));
    QString container = m_uiForm.dsContainer->getCurrentDataName();
    QString containerType =
        container.right(container.length() - container.lastIndexOf("_"));

    g_log.debug() << "Sample type is: " << sampleType.toStdString() << '\n';
    g_log.debug() << "Can type is: " << containerType.toStdString() << '\n';

    if (containerType != sampleType)
      uiv.addErrorMessage(
          "Sample and can workspaces must contain the same type of data.");
  }

  if (useCorrections) {
    if (m_uiForm.dsCorrections->getCurrentDataName().compare("") == 0) {
      uiv.addErrorMessage(
          "Use Correction must contain a corrections file or workspace.");
    } else {

      QString correctionsWsName = m_uiForm.dsCorrections->getCurrentDataName();
      WorkspaceGroup_sptr corrections =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
              correctionsWsName.toStdString());
      for (size_t i = 0; i < corrections->size(); i++) {
        // Check it is a MatrixWorkspace
        MatrixWorkspace_sptr factorWs =
            boost::dynamic_pointer_cast<MatrixWorkspace>(
                corrections->getItem(i));
        if (!factorWs) {
          QString msg = "Correction factor workspace " + QString::number(i) +
                        " is not a MatrixWorkspace";
          uiv.addErrorMessage(msg);
          continue;
        }

        // Check X unit is wavelength
        Mantid::Kernel::Unit_sptr xUnit = factorWs->getAxis(0)->unit();
        if (xUnit->caption() != "Wavelength") {
          QString msg = "Correction factor workspace " +
                        QString::fromStdString(factorWs->getName()) +
                        " is not in wavelength";
          uiv.addErrorMessage(msg);
        }
      }
    }
  }

  // Show errors if there are any
  if (!uiv.isAllInputValid())
    emit showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

void ApplyPaalmanPings::loadSettings(const QSettings &settings) {
  m_uiForm.dsCorrections->readSettings(settings.group());
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}

/**
* Handles when the type of geometry changes
*
* Updates the file extension to search for
*/
void ApplyPaalmanPings::handleGeometryChange(int index) {
  QString ext("");
  switch (index) {
  case 0:
    // Geometry is flat
    ext = "_flt_abs";
    break;
  case 1:
    // Geometry is cylinder
    ext = "_cyl_abs";
    break;
  case 2:
    // Geometry is annulus
    ext = "_ann_abs";
    break;
  }
  m_uiForm.dsCorrections->setWSSuffixes(QStringList(ext));
  m_uiForm.dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));
}

/**
* Replots the preview plot.
*
* @param wsIndex Spectrum index to plot
*/
void ApplyPaalmanPings::plotPreview(int wsIndex) {
  bool useCan = m_uiForm.ckUseCan->isChecked();

  m_uiForm.ppPreview->clear();

  // Plot sample
  m_uiForm.ppPreview->addSpectrum("Sample",
                                  QString::fromStdString(m_sampleWorkspaceName),
                                  wsIndex, Qt::black);

  // Plot result
  if (!m_pythonExportWsName.empty())
    m_uiForm.ppPreview->addSpectrum(
        "Corrected", QString::fromStdString(m_pythonExportWsName), wsIndex,
        Qt::green);
  // Plot container
  if (useCan) {
    m_uiForm.ppPreview->addSpectrum(
        "Container", QString::fromStdString(m_containerWorkspaceName), wsIndex,
        Qt::red);
  }
}
/**
 * Handles saving of the workspace
 */
void ApplyPaalmanPings::saveClicked() {

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles mantid plotting of workspace
 */
void ApplyPaalmanPings::plotClicked() {

  QString plotType = m_uiForm.cbPlotOutput->currentText();

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, true)) {

    if (plotType == "Spectra" || plotType == "Both")
      plotSpectrum(QString::fromStdString(m_pythonExportWsName));

    if (plotType == "Contour" || plotType == "Both")
      plot2D(QString::fromStdString(m_pythonExportWsName));
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt
