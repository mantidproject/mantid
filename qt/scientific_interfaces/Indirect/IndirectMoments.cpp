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

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectMoments::IndirectMoments(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent), m_model(std::make_unique<IndirectMomentsModel>()),
      m_view(std::make_unique<IndirectMomentsView>(parent)) {
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra, "0,2,4"));

  m_view->setupProperties();

  connect(m_view.get(), SIGNAL(dataReady(QString const &)), this, SLOT(handleDataReady(const QString &)));
  connect(m_view.get(), SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
  connect(m_view.get(), SIGNAL(scaleChanged(int)), this, SLOT(handleScaleChanged(int)));
  connect(m_view.get(), SIGNAL(scaleValueChanged(double)), this, SLOT(handleScaleValueChanged(double)));
  connect(m_view.get(), SIGNAL(runClicked()), this, SLOT(runClicked()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveClicked()));

  // Update the preview plot when the algorithm completes
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(momentsAlgComplete(bool)));

  connect(this, SIGNAL(updateRunButton(bool, std::string const &, QString const &, QString const &)), m_view.get(),
          SLOT(updateRunButton(bool, std::string const &, QString const &, QString const &)));

  connect(m_view.get(), SIGNAL(showMessageBox(const QString &)), this, SIGNAL(showMessageBox(const QString &)));
}

void IndirectMoments::setup() {}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 */
void IndirectMoments::handleDataReady(QString const &dataName) {
  if (m_view->validate()) {
    m_model->setInputWorkspace(m_view->getDataName());
    plotNewData(dataName);
  }
}

/**
 * Handles the scale checkbox being changed.
 */
void IndirectMoments::handleScaleChanged(int state) { m_model->setScale(state == Qt::Checked); }

/**
 * Handles the scale value being changed.
 */
void IndirectMoments::handleScaleValueChanged(double value) { m_model->setScaleValue(value); }

void IndirectMoments::run() { runAlgorithm(m_model->setupAlgorithm()); }

bool IndirectMoments::validate() { return true; }
/**
 * Clears previous plot data (in both preview and raw plot) and sets the new
 * range bars
 */
void IndirectMoments::plotNewData(QString const &filename) {

  m_view->plotNewData(filename);
  auto const range = getXRangeFromWorkspace(filename.toStdString());
  m_view->setPlotPropertyRange(range);
  m_view->setRangeSelector(range);
  m_view->replot();
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
  if (prop->propertyName() == "EMin") {
    m_model->setEMin(val);
  } else if (prop->propertyName() == "EMax") {
    m_model->setEMax(val);
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

  MatrixWorkspace_sptr outputWorkspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_model->getOutputWorkspace());

  if (outputWorkspace->getNumberHistograms() < 5)
    return;

  setOutputPlotOptionsWorkspaces({m_model->getOutputWorkspace()});

  m_view->plotOutput(QString::fromStdString(m_model->getOutputWorkspace()));
}

void IndirectMoments::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Moments");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

/**
 * Handle when Run is clicked
 */
void IndirectMoments::runClicked() { runTab(); }

/**
 * Handles saving of workspaces
 */
void IndirectMoments::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_model->getOutputWorkspace(), false))
    addSaveWorkspaceToQueue(m_model->getOutputWorkspace());
  m_batchAlgoRunner->executeBatchAsync();
}

} // namespace MantidQt::CustomInterfaces
