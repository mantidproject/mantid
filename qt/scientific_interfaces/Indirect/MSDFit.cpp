#include "MSDFit.h"
#include "../General/UserInputValidator.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

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
    : IndirectFitAnalysisTab(new MSDFitModel, parent),
      m_uiForm(new Ui::MSDFit) {
  m_uiForm->setupUi(parent);

  m_msdFittingModel = dynamic_cast<MSDFitModel *>(fittingModel());
  setFitDataPresenter(Mantid::Kernel::make_unique<IndirectFitDataPresenter>(
      m_msdFittingModel, m_uiForm->fitDataView));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
}

void MSDFit::setupFitTab() {
  auto &functionFactory = FunctionFactory::Instance();
  auto gaussian = functionFactory.createFunction("MSDGauss");
  auto peters = functionFactory.createFunction("MSDPeters");
  auto yi = functionFactory.createFunction("MSDYi");
  addComboBoxFunctionGroup("Gaussian", {gaussian});
  addComboBoxFunctionGroup("Peters", {peters});
  addComboBoxFunctionGroup("Yi", {yi});

  setSampleWSSuffices({"_eq"});
  setSampleFBSuffices({"_eq.nxs"});

  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
}

void MSDFit::updateModelFitTypeString() {
  m_msdFittingModel->setFitType(selectedFitType().toStdString());
}

void MSDFit::updatePlotOptions() {}

void MSDFit::plotClicked() {
  setPlotResultIsPlotting(true);
  IndirectFitAnalysisTab::plotResult("All");
  setPlotResultIsPlotting(false);
}

bool MSDFit::shouldEnablePlotResult() {
  for (auto i = 0u; i < m_msdFittingModel->numberOfWorkspaces(); ++i)
    if (m_msdFittingModel->getNumberOfSpectra(i) > 1)
      return true;
  return false;
}

void MSDFit::setRunEnabled(bool enabled) {
  m_uiForm->pbRun->setEnabled(enabled);
}

void MSDFit::setPlotResultEnabled(bool enabled) {
  m_uiForm->pbPlot->setEnabled(enabled);
}

void MSDFit::setFitSingleSpectrumEnabled(bool enabled) {
  m_uiForm->pvFitPlotView->enableFitSingleSpectrum(enabled);
}

void MSDFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

void MSDFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
  setRunEnabled(!running);
  setFitSingleSpectrumEnabled(!running);
}

void MSDFit::setPlotResultIsPlotting(bool plotting) {
  m_uiForm->pbPlot->setText(plotting ? "Plotting..." : "Plot Result");
  setPlotResultEnabled(!plotting);
}

void MSDFit::runClicked() { runTab(); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
