// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MomentsTab.h"
#include "Common/IndirectDataValidationHelper.h"
#include "Common/InterfaceUtils.h"
#include "Common/WorkspaceUtils.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MomentsTab::MomentsTab(QWidget *parent, IMomentsView *view)
    : DataManipulationTab(parent), m_model(std::make_unique<MomentsTabModel>()), m_view(view) {
  m_view->subscribePresenter(this);
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra, "0,2,4"));
}

void MomentsTab::setup() {}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 */
void MomentsTab::handleDataReady(std::string const &dataName) {
  if (m_view->validate()) {
    m_model->setInputWorkspace(m_view->getDataName());
    plotNewData(dataName);
  }
}

/**
 * Handles the scale checkbox being changed.
 */
void MomentsTab::handleScaleChanged(bool state) { m_model->setScale(state); }

/**
 * Handles the scale value being changed.
 */
void MomentsTab::handleScaleValueChanged(double value) { m_model->setScaleValue(value); }

void MomentsTab::run() { runAlgorithm(m_model->setupAlgorithm()); }

bool MomentsTab::validate() { return true; }
/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 */

void MomentsTab::plotNewData(std::string const &filename) {

  m_view->plotNewData(filename);
  auto const range = WorkspaceUtils::getXRangeFromWorkspace(filename);
  m_view->setPlotPropertyRange(range);
  m_view->setRangeSelector(range);
  m_view->replot();
}

/**
 * Handles when numeric value of properties in the property manager are updated.
 *
 * Performs validation and updated preview plot.
 *
 * @param propName :: The property being updated
 * @param val :: The new value for the property
 */
void MomentsTab::handleValueChanged(std::string const &propName, double value) {
  if (propName == "EMin") {
    m_model->setEMin(value);
  } else if (propName == "EMax") {
    m_model->setEMax(value);
  }
}

/**
 * Handles plotting the preview plot when the algorithm finishes.
 *
 * @param error True if the algorithm exited due to error, false otherwise
 */
void MomentsTab::runComplete(bool error) {
  if (error)
    return;

  MatrixWorkspace_sptr outputWorkspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_model->getOutputWorkspace());

  if (outputWorkspace->getNumberHistograms() < 5)
    return;

  setOutputPlotOptionsWorkspaces({m_model->getOutputWorkspace()});

  m_view->plotOutput(m_model->getOutputWorkspace());
  m_view->getPlotOptions()->setIndicesLineEditEnabled(true);
}

void MomentsTab::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Moments");
  m_view->setFBSuffixes(filter ? InterfaceUtils::getSampleFBSuffixes(tabName) : InterfaceUtils::getExtensions(tabName));
  m_view->setWSSuffixes(filter ? InterfaceUtils::getSampleWSSuffixes(tabName) : noSuffixes);
}

/**
 * Handle when Run is clicked
 */
void MomentsTab::handleRunClicked() { runTab(); }

/**
 * Handles saving of workspaces
 */
void MomentsTab::handleSaveClicked() {
  if (checkADSForPlotSaveWorkspace(m_model->getOutputWorkspace(), false))
    addSaveWorkspaceToQueue(m_model->getOutputWorkspace());
  m_batchAlgoRunner->executeBatchAsync();
}

} // namespace MantidQt::CustomInterfaces
