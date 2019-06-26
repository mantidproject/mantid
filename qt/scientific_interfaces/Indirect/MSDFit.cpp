// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFit.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid;
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
  setFitDataPresenter(std::make_unique<IndirectFitDataPresenter>(
      m_msdFittingModel, m_uiForm->fitDataView));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);

  setEditResultVisible(false);
}

void MSDFit::setupFitTab() {
  auto &functionFactory = FunctionFactory::Instance();
  auto gaussian = functionFactory.createFunction("MSDGauss");
  auto peters = functionFactory.createFunction("MSDPeters");
  auto yi = functionFactory.createFunction("MSDYi");
  //addComboBoxFunctionGroup("Gaussian", {gaussian});
  //addComboBoxFunctionGroup("Peters", {peters});
  //addComboBoxFunctionGroup("Yi", {yi});

  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
}

EstimationDataSelector MSDFit::getEstimationDataSelector() const {
  return
      [](const MantidVec &x, const MantidVec &y) -> DataForParameterEstimation {
        return DataForParameterEstimation{{}, {}};
      };
}

void MSDFit::updateModelFitTypeString() {
  m_msdFittingModel->setFitType(selectedFitType().toStdString());
}

void MSDFit::runClicked() { runTab(); }

void MSDFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void MSDFit::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
