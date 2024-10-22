// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MomentsPresenter.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
auto &ads = AnalysisDataService::Instance();
}

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MomentsPresenter::MomentsPresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner,
                                   IMomentsView *view, std::unique_ptr<IMomentsModel> model)
    : DataProcessor(parent, std::move(algorithmRunner)), m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra, "0,2,4"));
}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 */
void MomentsPresenter::handleDataReady(std::string const &dataName) {
  if (m_runPresenter->validate()) {
    m_model->setInputWorkspace(m_view->getDataName());
    plotNewData(dataName);
  }
}

/**
 * Handles the scale checkbox being changed.
 */
void MomentsPresenter::handleScaleChanged(bool state) { m_model->setScale(state); }

/**
 * Handles the scale value being changed.
 */
void MomentsPresenter::handleScaleValueChanged(double value) { m_model->setScaleValue(value); }

void MomentsPresenter::handleValidation(IUserInputValidator *validator) const {
  validateDataIsOfType(validator, m_view->getDataSelector(), "Sample", DataType::Sqw);
}

/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 */

void MomentsPresenter::plotNewData(std::string const &filename) {

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
void MomentsPresenter::handleValueChanged(std::string const &propName, double value) {
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
void MomentsPresenter::runComplete(Mantid::API::IAlgorithm_sptr algorithm, bool error) {
  if (error)
    return;

  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  if (outputWorkspace->getNumberHistograms() < 5)
    return;

  m_view->plotOutput(outputWorkspace);

  auto const outputName = m_model->getOutputWorkspace();
  ads.addOrReplace(outputName, outputWorkspace);
  setOutputPlotOptionsWorkspaces({outputName});
}

void MomentsPresenter::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Moments");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

void MomentsPresenter::setLoadHistory(bool doLoadHistory) { m_view->setLoadHistory(doLoadHistory); }

/**
 * Handle when Run is clicked
 */
void MomentsPresenter::handleRun() {
  clearOutputPlotOptionsWorkspaces();
  m_algorithmRunner->execute(m_model->setupMomentsAlgorithm());
}

/**
 * Handles saving of workspaces
 */
void MomentsPresenter::handleSaveClicked() {
  if (checkADSForPlotSaveWorkspace(m_model->getOutputWorkspace(), false))
    m_algorithmRunner->execute(setupSaveAlgorithm(m_model->getOutputWorkspace()));
}

} // namespace MantidQt::CustomInterfaces
