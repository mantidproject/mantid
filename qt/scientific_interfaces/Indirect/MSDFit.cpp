// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFit.h"
#include "IndirectFunctionBrowser/MSDTemplateBrowser.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QFileInfo>

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
  auto templateBrowser = new MSDTemplateBrowser;
  m_uiForm->fitPropertyBrowser->setFunctionTemplateBrowser(templateBrowser);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);

  setEditResultVisible(false);
  m_uiForm->fitDataView->setStartAndEndHidden(false);
}

void MSDFit::setupFitTab() {
  auto &functionFactory = FunctionFactory::Instance();
  auto gaussian = functionFactory.createFunction("MSDGauss");
  auto peters = functionFactory.createFunction("MSDPeters");
  auto yi = functionFactory.createFunction("MSDYi");

  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
}

void MSDFit::runClicked() { runTab(); }

void MSDFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void MSDFit::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

EstimationDataSelector MSDFit::getEstimationDataSelector() const {
  return [](const std::vector<double> &,
            const std::vector<double> &) -> DataForParameterEstimation {
    return DataForParameterEstimation{};
  };
}

void MSDFit::fitFunctionChanged() {
  m_msdFittingModel->setFitTypeString(fitTypeString());
}

std::string MSDFit::fitTypeString() const {
  // This function attempts to work out which fit type is being done. It will
  // currently only recognise the three default types.
  const auto numberOfGauss = numberOfCustomFunctions("MsdGauss");
  const auto numberOfPeters = numberOfCustomFunctions("MsdPeters");
  const auto numberOfYi = numberOfCustomFunctions("MsdYi");

  if (numberOfGauss + numberOfPeters + numberOfYi != 1) {
    return "UserDefined";
  }

  if (numberOfGauss == 1)
    return "Gauss";

  if (numberOfPeters == 1)
    return "Peters";

  if (numberOfYi == 1)
    return "Yi";

  return "UserDefined";
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
