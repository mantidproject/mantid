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

} // namespace MantidQt::CustomInterfaces::IDA
