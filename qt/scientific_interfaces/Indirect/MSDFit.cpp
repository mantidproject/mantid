// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFit.h"
#include "IndirectFunctionBrowser/SingleFunctionTemplateBrowser.h"

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
std::vector<std::string> MSDFIT_HIDDEN_PROPS = std::vector<std::string>(
    {"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
     "OutputCompositeMembers", "OutputWorkspace", "IgnoreInvalidData", "Output",
     "PeakRadius", "PlotParameter"});
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

auto msdFunctionStrings = std::map<std::string, std::string>(
    {{"Gauss", "name=MsdGauss,Height=1,Msd=0.05,constraints=(Height>0, Msd>0)"},
     {"Peters",
      "name=MsdPeters,Height=1,Msd=0.05,Beta=1,constraints=(Height>0, "
      "Msd>0, Beta>0)"},
     {"Yi",
      "name=MsdYi,Height=1,Msd=0.05,Sigma=1,constraints=(Height>0, Msd>0, "
      "Sigma>0)"}});

MSDFit::MSDFit(QWidget *parent)
    : IndirectFitAnalysisTab(new MSDFitModel, parent),
      m_uiForm(new Ui::IndirectFitTab) {
  m_uiForm->setupUi(parent);

  m_msdFittingModel = dynamic_cast<MSDFitModel *>(fittingModel());
  setFitDataPresenter(std::make_unique<IndirectFitDataPresenter>(
      m_msdFittingModel, m_uiForm->dockArea->m_fitDataView));
  setPlotView(m_uiForm->dockArea->m_fitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  auto templateBrowser = new SingleFunctionTemplateBrowser(msdFunctionStrings);
  m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplateBrowser(
      templateBrowser);
  setFitPropertyBrowser(m_uiForm->dockArea->m_fitPropertyBrowser);
  m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(
      MSDFIT_HIDDEN_PROPS);

  setEditResultVisible(false);
  respondToFunctionChanged();
  fitFunctionChanged();
}

void MSDFit::setupFitTab() {
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
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
