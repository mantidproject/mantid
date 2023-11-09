// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisMSDFitTab.h"
#include "FitTabConstants.h"
#include "MSDFitModel.h"

#include "FunctionBrowser/MSDTemplateBrowser.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataAnalysisMSDFitTab::IndirectDataAnalysisMSDFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new MSDFitModel, new MSDTemplateBrowser, new IndirectFitDataView, MSDFit::HIDDEN_PROPS,
                              parent) {
  setupFitDataPresenter<IndirectFitDataPresenter>();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
