// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationMomentsTab.h"
#include "IndirectDataValidationHelper.h"

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
InelasticDataManipulationMomentsTab::InelasticDataManipulationMomentsTab(QWidget *parent, IMomentsView *view)
    : InelasticDataManipulationTab(parent), m_model(std::make_unique<InelasticDataManipulationMomentsTabModel>()),
      m_view(view) {
  m_view->subscribePresenter(this);
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra, "0,2,4"));
}

void InelasticDataManipulationMomentsTab::setup() {}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 */
void InelasticDataManipulationMomentsTab::handleDataReady(std::string const &dataName) {
  if (m_view->validate()) {
    m_model->setInputWorkspace(m_view->getDataName());
    plotNewData(dataName);
  }
}

/**
 * Handles the scale checkbox being changed.
 */
void InelasticDataManipulationMomentsTab::handleScaleChanged(bool state) { m_model->setScale(state); }

/**
 * Handles the scale value being changed.
 */
void InelasticDataManipulationMomentsTab::handleScaleValueChanged(double value) { m_model->setScaleValue(value); }

void InelasticDataManipulationMomentsTab::run() { runAlgorithm(m_model->setupAlgorithm()); }

bool InelasticDataManipulationMomentsTab::validate() { return true; }
/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 */

void InelasticDataManipulationMomentsTab::plotNewData(std::string const &filename) {

  m_view->plotNewData(filename);
  auto const range = getXRangeFromWorkspace(filename);
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
void InelasticDataManipulationMomentsTab::handleValueChanged(std::string const &propName, double value) {
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
void InelasticDataManipulationMomentsTab::runComplete(bool error) {
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

void InelasticDataManipulationMomentsTab::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Moments");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

/**
 * Handle when Run is clicked
 */
void InelasticDataManipulationMomentsTab::handleRunClicked() { runTab(); }

/**
 * Handles saving of workspaces
 */
void InelasticDataManipulationMomentsTab::handleSaveClicked() {
  if (checkADSForPlotSaveWorkspace(m_model->getOutputWorkspace(), false))
    addSaveWorkspaceToQueue(m_model->getOutputWorkspace());
  m_batchAlgoRunner->executeBatchAsync();
}

} // namespace MantidQt::CustomInterfaces
