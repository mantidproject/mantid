// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisIqtFitTab.h"
#include "FitTabConstants.h"
#include "FunctionBrowser/IqtTemplateBrowser.h"
#include "IqtFitModel.h"

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisIqtFitTab::IndirectDataAnalysisIqtFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new IqtFitModel, new IqtTemplateBrowser, new IndirectFitDataView, IqtFit::HIDDEN_PROPS,
                              parent) {
  setupFitDataPresenter<IndirectFitDataPresenter>();
}

EstimationDataSelector IndirectDataAnalysisIqtFitTab::getEstimationDataSelector() const {
  return [](const Mantid::MantidVec &x, const Mantid::MantidVec &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    (void)range;
    size_t const n = 4;
    if (y.size() < n + 1)
      return DataForParameterEstimation{{}, {}};
    return DataForParameterEstimation{{x[0], x[n]}, {y[0], y[n]}};
  };
}

} // namespace MantidQt::CustomInterfaces::IDA
