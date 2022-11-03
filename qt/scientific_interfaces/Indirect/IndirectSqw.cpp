// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSqw.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/AxisID.h"

#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace {
Mantid::Kernel::Logger g_log("S(Q,w)");

} // namespace

namespace MantidQt::CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectSqw::IndirectSqw(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent), m_model(std::make_unique<IndirectSqwModel>()),
      m_view(std::make_unique<IndirectSqwView>(parent)) {
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::SpectraContour));
  connectSignals();
}

void IndirectSqw::setup() {}

/**
 * Connects the signals in the interface.
 *
 */

void IndirectSqw::connectSignals() {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(sqwAlgDone(bool)));

  connect(m_view.get(), SIGNAL(dataReady(QString const &)), this, SLOT(handleDataReady(QString const &)));
  connect(m_view.get(), SIGNAL(qLowChanged(double)), this, SLOT(qLowChanged(double)));
  connect(m_view.get(), SIGNAL(qWidthChanged(double)), this, SLOT(qWidthChanged(double)));
  connect(m_view.get(), SIGNAL(qHighChanged(double)), this, SLOT(qHighChanged(double)));
  connect(m_view.get(), SIGNAL(eLowChanged(double)), this, SLOT(eLowChanged(double)));
  connect(m_view.get(), SIGNAL(eWidthChanged(double)), this, SLOT(eWidthChanged(double)));
  connect(m_view.get(), SIGNAL(eHighChanged(double)), this, SLOT(eHighChanged(double)));
  connect(m_view.get(), SIGNAL(rebinEChanged(int)), this, SLOT(rebinEChanged(int)));

  connect(m_view.get(), SIGNAL(runClicked()), this, SLOT(runClicked()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveClicked()));

  connect(m_view.get(), SIGNAL(showMessageBox(const QString &)), this, SIGNAL(showMessageBox(const QString &)));

  connect(this, SIGNAL(updateRunButton(bool, std::string const &, QString const &, QString const &)), m_view.get(),
          SLOT(updateRunButton(bool, std::string const &, QString const &, QString const &)));
}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 */
void IndirectSqw::handleDataReady(QString const &dataName) {
  if (m_view->validate()) {
    m_model->setInputWorkspace(dataName.toStdString());
    try {
      auto const eFixed = getInstrumentDetail("Efixed").toStdString();
      m_model->setEFixed(eFixed);
    } catch (std::runtime_error const &ex) {
      emit showMessageBox(ex.what());
      return;
    }
    plotRqwContour();
    m_view->setDefaultQAndEnergy();
  }
}

bool IndirectSqw::validate() {
  UserInputValidator uiv = m_model->validate(m_view->getQRangeFromPlot(), m_view->getERangeFromPlot());
  auto const errorMessage = uiv.generateErrorMessage();
  // Show an error message if needed
  if (!errorMessage.isEmpty())
    emit showMessageBox(errorMessage);
  return errorMessage.isEmpty();
}

void IndirectSqw::run() {
  m_model->setupRebinAlgorithm(m_batchAlgoRunner);
  m_model->setupSofQWAlgorithm(m_batchAlgoRunner);
  m_model->setupAddSampleLogAlgorithm(m_batchAlgoRunner);

  m_batchAlgoRunner->executeBatch();
}

/**
 * Handles plotting the S(Q, w) workspace when the algorithm chain is finished.
 *
 * @param error If the algorithm chain failed
 */
void IndirectSqw::sqwAlgDone(bool error) {
  if (!error) {
    setOutputPlotOptionsWorkspaces({m_model->getOutputWorkspace()});
    m_view->setSaveEnabled(true);
  }
}

/**
 * Plots the data as a contour plot
 *
 * Creates a colour 2D plot of the data
 */
void IndirectSqw::plotRqwContour() {
  try {
    auto const rqwWorkspace = m_model->getRqwWorkspace();
    if (rqwWorkspace)
      m_view->plotRqwContour(rqwWorkspace);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
    showMessageBox("Invalid file. Please load a valid reduced workspace.");
  }
}

void IndirectSqw::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Sqw");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

void IndirectSqw::runClicked() { runTab(); }

void IndirectSqw::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_model->getOutputWorkspace(), false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_model->getOutputWorkspace()));
  m_batchAlgoRunner->executeBatch();
}

void IndirectSqw::qLowChanged(double value) { m_model->setQMin(value); }

void IndirectSqw::qWidthChanged(double value) { m_model->setQWidth(value); }

void IndirectSqw::qHighChanged(double value) { m_model->setQMax(value); }

void IndirectSqw::eLowChanged(double value) { m_model->setEMin(value); }

void IndirectSqw::eWidthChanged(double value) { m_model->setEWidth(value); }

void IndirectSqw::eHighChanged(double value) { m_model->setEMax(value); }

void IndirectSqw::rebinEChanged(int value) { m_model->setRebinInEnergy(value != 0); }
} // namespace MantidQt::CustomInterfaces
