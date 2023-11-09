// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisFqFitTab.h"
#include "FitTabConstants.h"
#include "FqFitDataPresenter.h"
#include "FqFitModel.h"

#include "FunctionBrowser/FqTemplateBrowser.h"

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisFqFitTab::IndirectDataAnalysisFqFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new FqFitModel, new FqTemplateBrowser, new FqFitDataView, FqFit::HIDDEN_PROPS, parent) {
  setupFitDataPresenter<FqFitDataPresenter>();
  m_plotPresenter->setXBounds({0.0, 2.0});
}

} // namespace MantidQt::CustomInterfaces::IDA
