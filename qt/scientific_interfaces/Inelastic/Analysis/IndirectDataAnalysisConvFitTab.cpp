// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisConvFitTab.h"
#include "ConvFitDataPresenter.h"
#include "FitTabConstants.h"
#include "FunctionBrowser/ConvTemplateBrowser.h"

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisConvFitTab::IndirectDataAnalysisConvFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new ConvFitModel, new ConvTemplateBrowser, new ConvFitDataView, ConvFit::HIDDEN_PROPS,
                              parent) {
  setupFitDataPresenter<ConvFitDataPresenter>();
  setConvolveMembers(true);
}

} // namespace MantidQt::CustomInterfaces::IDA
