#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MSDFit.h"
#include "../General/UserInputValidator.h"
#include "MantidQtWidgets/Common/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("MSDFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
MSDFit::MSDFit(QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_msdTree(nullptr), m_msdInputWS() {
  m_uiForm.setupUi(parent);
}

void MSDFit::setup() {
  // Tree Browser
  m_msdTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_msdTree);

  m_msdTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  m_properties["Start"] = m_dblManager->addProperty("StartX");
  m_dblManager->setDecimals(m_properties["Start"], NUM_DECIMALS);
  m_properties["End"] = m_dblManager->addProperty("EndX");
  m_dblManager->setDecimals(m_properties["End"], NUM_DECIMALS);

  m_msdTree->addProperty(m_properties["Start"]);
  m_msdTree->addProperty(m_properties["End"]);

  auto fitRangeSelector = m_uiForm.ppPlot->addRangeSelector("MSDRange");

  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minChanged(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxChanged(double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRS(QtProperty *, double)));

  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm.pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(plotInput()));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(plotFit()));

  connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(plotFit()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
}

void MSDFit::run() {
  if (!validate())
    return;

  // Set the result workspace for Python script export
  QString model = m_uiForm.modelInput->currentText();
  QString dataName = m_uiForm.dsSampleInput->getCurrentDataName();
  m_pythonExportWsName =
      dataName.left(dataName.lastIndexOf("_")).toStdString() + "_" +
      model.toStdString() + "_msd";

  QString wsName = m_uiForm.dsSampleInput->getCurrentDataName();
  double xStart = m_dblManager->value(m_properties["Start"]);
  double xEnd = m_dblManager->value(m_properties["End"]);
  long specMin = m_uiForm.spSpectraMin->value();
  long specMax = m_uiForm.spSpectraMax->value();

  IAlgorithm_sptr msdAlg = AlgorithmManager::Instance().create("MSDFit");
  msdAlg->initialize();
  msdAlg->setProperty("Model", model.toStdString());
  msdAlg->setProperty("InputWorkspace", wsName.toStdString());
  msdAlg->setProperty("XStart", xStart);
  msdAlg->setProperty("XEnd", xEnd);
  msdAlg->setProperty("SpecMin", specMin);
  msdAlg->setProperty("SpecMax", specMax);
  msdAlg->setProperty("OutputWorkspace", m_pythonExportWsName);

  m_batchAlgoRunner->addAlgorithm(msdAlg);
  m_batchAlgoRunner->executeBatchAsync();

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
}

void MSDFit::singleFit() {
  if (!validate())
    return;

  // Set the result workspace for Python script export
  QString dataName = m_uiForm.dsSampleInput->getCurrentDataName();
  m_pythonExportWsName =
      dataName.left(dataName.lastIndexOf("_")).toStdString() + "_msd";

  QString wsName = m_uiForm.dsSampleInput->getCurrentDataName();
  double xStart = m_dblManager->value(m_properties["Start"]);
  double xEnd = m_dblManager->value(m_properties["End"]);
  long fitSpec = m_uiForm.spPlotSpectrum->value();

  IAlgorithm_sptr msdAlg = AlgorithmManager::Instance().create("MSDFit");
  msdAlg->initialize();
  msdAlg->setProperty("InputWorkspace", wsName.toStdString());
  msdAlg->setProperty("XStart", xStart);
  msdAlg->setProperty("XEnd", xEnd);
  msdAlg->setProperty("SpecMin", fitSpec);
  msdAlg->setProperty("SpecMax", fitSpec);
  msdAlg->setProperty("OutputWorkspace", m_pythonExportWsName);

  m_batchAlgoRunner->addAlgorithm(msdAlg);

  m_batchAlgoRunner->executeBatchAsync();
}

bool MSDFit::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample input", m_uiForm.dsSampleInput);

  auto range = std::make_pair(m_dblManager->value(m_properties["Start"]),
                              m_dblManager->value(m_properties["End"]));
  uiv.checkValidRange("a range", range);

  int specMin = m_uiForm.spSpectraMin->value();
  int specMax = m_uiForm.spSpectraMax->value();
  auto specRange = std::make_pair(specMin, specMax + 1);
  uiv.checkValidRange("spectrum range", specRange);

  QString errors = uiv.generateErrorMessage();
  showMessageBox(errors);

  return errors.isEmpty();
}

void MSDFit::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
}

/**
 * Handles the completion of the MSDFit algorithm
 *
 * @param error If the algorithm chain failed
 */
void MSDFit::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  if (error)
    return;

  // Enable plot and save
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

/**
 * Plots fitted data on the mini plot.
 *
 * @param wsName Name of fit _Workspaces workspace group (defaults to
 *               Python export WS name + _Workspaces)
 * @param specNo Spectrum number relating to input workspace to plot fit
 *               for (defaults to value of preview spectrum index)
 */
void MSDFit::plotFit(QString wsName, int specNo) {
  if (wsName.isEmpty())
    wsName = QString::fromStdString(m_pythonExportWsName) + "_Workspaces";

  if (specNo == -1)
    specNo = m_uiForm.spPlotSpectrum->value();

  if (Mantid::API::AnalysisDataService::Instance().doesExist(
          wsName.toStdString())) {
    // Remove the old fit
    m_uiForm.ppPlot->removeSpectrum("Fit");

    // Get the workspace
    auto groupWs =
        AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>(
            wsName.toStdString());
    auto groupWsNames = groupWs->getNames();

    // Find the correct fit workspace and plot it
    std::stringstream searchString;
    searchString << "_" << specNo << "_Workspace";
    for (auto it = groupWsNames.begin(); it != groupWsNames.end(); ++it) {
      std::string wsName = *it;
      if (wsName.find(searchString.str()) != std::string::npos) {
        // Get the fit workspace
        auto ws =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
        m_msdInputWS = ws;
        // Plot the new fit
        m_uiForm.ppPlot->addSpectrum("Fit", ws, 1, Qt::red);
        // Nothing else to do
        return;
      }
    }
  }
}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void MSDFit::newDataLoaded(const QString wsName) {
  m_msdInputWS =
      Mantid::API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          wsName.toStdString());
  int maxWsIndex = static_cast<int>(m_msdInputWS->getNumberHistograms()) - 1;

  m_uiForm.spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm.spPlotSpectrum->setMinimum(0);
  m_uiForm.spPlotSpectrum->setValue(0);

  m_uiForm.spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMin->setMinimum(0);

  m_uiForm.spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMax->setMinimum(0);
  m_uiForm.spSpectraMax->setValue(maxWsIndex);

  plotInput();
}

/**
 * Plot the supplied input workspace in the mini-plot.
 */
void MSDFit::plotInput() {
  m_uiForm.ppPlot->clear();

  int wsIndex = m_uiForm.spPlotSpectrum->value();
  m_uiForm.ppPlot->addSpectrum("Sample", m_msdInputWS, wsIndex);

  try {
    QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
    m_uiForm.ppPlot->getRangeSelector("MSDRange")
        ->setRange(range.first, range.second);
  } catch (std::invalid_argument &exc) {
    showMessageBox(exc.what());
  }
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void MSDFit::specMinChanged(int value) {
  m_uiForm.spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void MSDFit::specMaxChanged(int value) {
  m_uiForm.spSpectraMin->setMaximum(value);
}

void MSDFit::minChanged(double val) {
  m_dblManager->setValue(m_properties["Start"], val);
}

void MSDFit::maxChanged(double val) {
  m_dblManager->setValue(m_properties["End"], val);
}

void MSDFit::updateRS(QtProperty *prop, double val) {
  auto fitRangeSelector = m_uiForm.ppPlot->getRangeSelector("MSDRange");

  if (prop == m_properties["Start"])
    fitRangeSelector->setMinimum(val);
  else if (prop == m_properties["End"])
    fitRangeSelector->setMaximum(val);
}

/**
 * Handles saving of workspace
 */
void MSDFit::saveClicked() {

  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles mantid plotting
 */
void MSDFit::plotClicked() {
  auto wsName = QString::fromStdString(m_pythonExportWsName) + "_Workspaces";
  if (checkADSForPlotSaveWorkspace(wsName.toStdString(), true)) {
    // Get the workspace
    auto groupWs =
        AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>(
            wsName.toStdString());
    auto groupWsNames = groupWs->getNames();
    if (groupWsNames.size() != 1) {
      plotSpectrum(QString::fromStdString(m_pythonExportWsName), 1);
    }

    else
      plotSpectrum(wsName, 0, 2);
  }
}

/**
* Plots the current spectrum displayed in the preview plot
*/
void MSDFit::plotCurrentPreview() {

  // Check a workspace has been selected
  if (m_msdInputWS) {
    const auto workspaceIndex = m_uiForm.spPlotSpectrum->value();
    IndirectTab::plotSpectrum(QString::fromStdString(m_msdInputWS->getName()),
                              workspaceIndex, workspaceIndex);
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
