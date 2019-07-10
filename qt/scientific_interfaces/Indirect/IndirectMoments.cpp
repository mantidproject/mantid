// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectMoments.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectMoments::IndirectMoments(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  const unsigned int NUM_DECIMALS = 6;

  m_uiForm.ppRawPlot->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppMomentsPreview->setCanvasColour(QColor(240, 240, 240));

  MantidWidgets::RangeSelector *xRangeSelector =
      m_uiForm.ppRawPlot->addRangeSelector("XRange");

  // PROPERTY TREE
  m_propTrees["MomentsPropTree"] = new QtTreePropertyBrowser();
  m_propTrees["MomentsPropTree"]->setFactoryForManager(m_dblManager,
                                                       m_dblEdFac);
  m_uiForm.properties->addWidget(m_propTrees["MomentsPropTree"]);
  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");

  m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMin"]);
  m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMax"]);

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

  connect(m_uiForm.dsInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleSampleInputReady(const QString &)));

  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this,
          SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateProperties(QtProperty *, double)));

  // Update the preview plot when the algorithm completes
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(momentsAlgComplete(bool)));

  // Plot and save
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));

  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectMoments::~IndirectMoments() {}

void IndirectMoments::setup() {}

void IndirectMoments::run() {
  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  QString outputName = workspaceName.left(workspaceName.length() - 4);
  double scale = m_uiForm.spScale->value();
  double eMin = m_dblManager->value(m_properties["EMin"]);
  double eMax = m_dblManager->value(m_properties["EMax"]);

  std::string outputWorkspaceName = outputName.toStdString() + "_Moments";

  IAlgorithm_sptr momentsAlg =
      AlgorithmManager::Instance().create("SofQWMoments", -1);
  momentsAlg->initialize();
  momentsAlg->setProperty("InputWorkspace", workspaceName.toStdString());
  momentsAlg->setProperty("EnergyMin", eMin);
  momentsAlg->setProperty("EnergyMax", eMax);
  momentsAlg->setProperty("OutputWorkspace", outputWorkspaceName);

  if (m_uiForm.ckScale->isChecked())
    momentsAlg->setProperty("Scale", scale);

  // Set the workspace name for Python script export
  m_pythonExportWsName = outputWorkspaceName;

  // Execute algorithm on separate thread
  runAlgorithm(momentsAlg);
}

bool IndirectMoments::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample input", m_uiForm.dsInput);

  QString msg = uiv.generateErrorMessage();
  if (!msg.isEmpty()) {
    emit showMessageBox(msg);
    return false;
  }

  return true;
}

/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 */
void IndirectMoments::handleSampleInputReady(const QString &filename) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));

  // Clears previous plotted data
  m_uiForm.ppRawPlot->clear();
  m_uiForm.ppMomentsPreview->clear();

  // Update plot and change data in interface
  m_uiForm.ppRawPlot->addSpectrum("Raw", filename, 0);
  auto const range = getXRangeFromWorkspace(filename.toStdString());

  auto xRangeSelector = m_uiForm.ppRawPlot->getRangeSelector("XRange");
  setPlotPropertyRange(xRangeSelector, m_properties["EMin"],
                       m_properties["EMax"], range);
  setRangeSelector(xRangeSelector, m_properties["EMin"], m_properties["EMax"],
                   range);
  m_uiForm.ppRawPlot->replot();

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Updates the property manager when the range selector is moved.
 *
 * @param min :: The new value of the lower guide
 * @param max :: The new value of the upper guide
 */
void IndirectMoments::rangeChanged(double min, double max) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));
  m_dblManager->setValue(m_properties["EMin"], min);
  m_dblManager->setValue(m_properties["EMax"], max);
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Handles when properties in the property manager are updated.
 *
 * Performs validation and updated preview plot.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void IndirectMoments::updateProperties(QtProperty *prop, double val) {
  if (prop == m_properties["EMin"]) {
    double emax = m_dblManager->value(m_properties["EMax"]);
    if (val > emax) {
      m_dblManager->setValue(prop, emax);
    } else {
      m_uiForm.ppRawPlot->getRangeSelector("XRange")->setMinimum(val);
      m_uiForm.ppRawPlot->replot();
    }
  } else if (prop == m_properties["EMax"]) {
    double emin = m_dblManager->value(m_properties["EMin"]);
    if (emin > val) {
      m_dblManager->setValue(prop, emin);
    } else {
      m_uiForm.ppRawPlot->getRangeSelector("XRange")->setMaximum(val);
      m_uiForm.ppRawPlot->replot();
    }
  }
}

/**
 * Handles plotting the preview plot when the algorithm finishes.
 *
 * @param error True if the algorithm exited due to error, false otherwise
 */
void IndirectMoments::momentsAlgComplete(bool error) {
  if (error)
    return;

  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  QString outputName = workspaceName.left(workspaceName.length() - 4);
  std::string outputWorkspaceName = outputName.toStdString() + "_Moments";

  MatrixWorkspace_sptr outputWorkspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          outputWorkspaceName);

  if (outputWorkspace->getNumberHistograms() < 5)
    return;

  // Plot each spectrum
  m_uiForm.ppMomentsPreview->clear();
  m_uiForm.ppMomentsPreview->addSpectrum(
      "M0", QString::fromStdString(outputWorkspaceName), 0, Qt::green);
  m_uiForm.ppMomentsPreview->addSpectrum(
      "M1", QString::fromStdString(outputWorkspaceName), 1, Qt::black);
  m_uiForm.ppMomentsPreview->addSpectrum(
      "M2", QString::fromStdString(outputWorkspaceName), 2, Qt::red);
  m_uiForm.ppMomentsPreview->resizeX();

  // Enable plot and save buttons
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

void IndirectMoments::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Moments");
  m_uiForm.dsInput->setFBSuffixes(filter ? getSampleFBSuffixes(tabName)
                                         : getExtensions(tabName));
  m_uiForm.dsInput->setWSSuffixes(filter ? getSampleWSSuffixes(tabName)
                                         : noSuffixes);
}

/**
 * Handle when Run is clicked
 */
void IndirectMoments::runClicked() { runTab(); }

/**
 * Handle mantid plotting
 */
void IndirectMoments::plotClicked() {
  setPlotIsPlotting(true);
  QString outputWs =
      getWorkspaceBasename(m_uiForm.dsInput->getCurrentDataName()) + "_Moments";
  if (checkADSForPlotSaveWorkspace(outputWs.toStdString(), true))
    plotSpectra(outputWs, {0, 2, 4});
  setPlotIsPlotting(false);
}

/**
 * Handles saving of workspaces
 */
void IndirectMoments::saveClicked() {
  QString outputWs =
      getWorkspaceBasename(m_uiForm.dsInput->getCurrentDataName()) + "_Moments";
  if (checkADSForPlotSaveWorkspace(outputWs.toStdString(), false))
    addSaveWorkspaceToQueue(outputWs);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectMoments::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}
void IndirectMoments::setPlotEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
}
void IndirectMoments::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectMoments::setOutputButtonsEnabled(
    std::string const &enableOutputButtons) {
  bool enable = enableOutputButtons == "enable" ? true : false;
  setPlotEnabled(enable);
  setSaveEnabled(enable);
}

void IndirectMoments::updateRunButton(bool enabled,
                                      std::string const &enableOutputButtons,
                                      QString const message,
                                      QString const tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setOutputButtonsEnabled(enableOutputButtons);
}

void IndirectMoments::setPlotIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot Result");
  setPlotEnabled(!plotting);
  setRunEnabled(!plotting);
  setSaveEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
