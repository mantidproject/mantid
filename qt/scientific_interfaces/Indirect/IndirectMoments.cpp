// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectMoments.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectMoments::IndirectMoments(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(std::make_unique<IndirectPlotOptionsPresenter>(
      m_uiForm.ipoPlotOptions, this, PlotWidget::Spectra, "0,2,4"));

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

  connect(m_uiForm.dsInput, SIGNAL(dataReady(QString const &)), this,
          SLOT(handleDataReady(const QString &)));

  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this,
          SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateProperties(QtProperty *, double)));

  // Update the preview plot when the algorithm completes
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(momentsAlgComplete(bool)));

  // Plot and save
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
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
IndirectMoments::~IndirectMoments() {
  m_propTrees["MomentsPropTree"]->unsetFactoryForManager(m_dblManager);
}

void IndirectMoments::setup() {}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 */
void IndirectMoments::handleDataReady(QString const &dataName) {
  if (validate())
    plotNewData(dataName);
}

bool IndirectMoments::validate() {
  UserInputValidator uiv;
  validateDataIsOfType(uiv, m_uiForm.dsInput, "Sample", DataType::Sqw);

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    showMessageBox(errorMessage);
  return errorMessage.isEmpty();
}

void IndirectMoments::run() {
  QString workspaceName = m_uiForm.dsInput->getCurrentDataName();
  QString outputName = workspaceName.left(workspaceName.length() - 4);
  double scale = m_uiForm.spScale->value();
  double eMin = m_dblManager->value(m_properties["EMin"]);
  double eMax = m_dblManager->value(m_properties["EMax"]);

  std::string const outputWorkspaceName = outputName.toStdString() + "_Moments";

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

/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 */
void IndirectMoments::plotNewData(QString const &filename) {
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
  auto eRangeSelector = m_uiForm.ppRawPlot->getRangeSelector("XRange");

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));

  if (prop == m_properties["EMin"]) {
    setRangeSelectorMin(m_properties["EMin"], m_properties["EMax"],
                        eRangeSelector, val);
  } else if (prop == m_properties["EMax"]) {
    setRangeSelectorMax(m_properties["EMin"], m_properties["EMax"],
                        eRangeSelector, val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Handles plotting the preview plot when the algorithm finishes.
 *
 * @param error True if the algorithm exited due to error, false otherwise
 */
void IndirectMoments::momentsAlgComplete(bool error) {
  if (error)
    return;

  MatrixWorkspace_sptr outputWorkspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_pythonExportWsName);

  if (outputWorkspace->getNumberHistograms() < 5)
    return;

  setOutputPlotOptionsWorkspaces({m_pythonExportWsName});

  // Plot each spectrum
  m_uiForm.ppMomentsPreview->clear();
  m_uiForm.ppMomentsPreview->addSpectrum(
      "M0", QString::fromStdString(m_pythonExportWsName), 0, Qt::green);
  m_uiForm.ppMomentsPreview->addSpectrum(
      "M1", QString::fromStdString(m_pythonExportWsName), 1, Qt::black);
  m_uiForm.ppMomentsPreview->addSpectrum(
      "M2", QString::fromStdString(m_pythonExportWsName), 2, Qt::red);
  m_uiForm.ppMomentsPreview->resizeX();

  // Enable plot and save buttons
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
 * Handles saving of workspaces
 */
void IndirectMoments::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(m_pythonExportWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectMoments::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectMoments::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectMoments::updateRunButton(bool enabled,
                                      std::string const &enableOutputButtons,
                                      QString const &message,
                                      QString const &tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setSaveEnabled(enableOutputButtons == "enable");
}

} // namespace CustomInterfaces
} // namespace MantidQt
