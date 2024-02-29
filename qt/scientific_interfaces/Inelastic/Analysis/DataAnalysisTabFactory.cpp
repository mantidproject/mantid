// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DataAnalysisTabFactory.h"

#include "ConvFitDataPresenter.h"
#include "FitTabConstants.h"
#include "FqFitDataPresenter.h"
#include "FqFitModel.h"
#include "FunctionBrowser/FqFunctionModel.h"
#include "FunctionBrowser/IqtFunctionModel.h"
#include "FunctionBrowser/IqtFunctionTemplateView.h"
#include "FunctionBrowser/IqtTemplatePresenter.h"
#include "FunctionBrowser/MSDFunctionModel.h"
#include "FunctionBrowser/MultiFunctionTemplateBrowser.h"
#include "FunctionBrowser/MultiFunctionTemplateModel.h"
#include "FunctionBrowser/MultiFunctionTemplatePresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateBrowser.h"
#include "FunctionBrowser/SingleFunctionTemplatePresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateView.h"
#include "IndirectDataAnalysisTab.h"
#include "IndirectFitDataPresenter.h"
#include "IqtFitModel.h"
#include "MSDFitModel.h"

namespace MantidQt::CustomInterfaces::IDA {

DataAnalysisTabFactory::DataAnalysisTabFactory(QTabWidget *tabWidget) : m_tabWidget(tabWidget) {}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeMSDFitTab(int const index) const {
  auto tab = new IndirectDataAnalysisTab(MSDFit::TAB_NAME, MSDFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<MSDFitModel>();
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, MSDFunctionModel>(
      MSDFit::HIDDEN_PROPS);
  tab->setupFitDataView<IndirectFitDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<IndirectFitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeIqtFitTab(int const index) const {
  auto tab = new IndirectDataAnalysisTab(IqtFit::TAB_NAME, IqtFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<IqtFitModel>();
  tab->setupFitPropertyBrowser<IqtFunctionTemplateView, IqtTemplatePresenter, IqtFunctionModel>(IqtFit::HIDDEN_PROPS);
  tab->setupFitDataView<IndirectFitDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<IndirectFitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeConvFitTab(int const index) const {
  auto tab = new IndirectDataAnalysisTab(ConvFit::TAB_NAME, ConvFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<ConvFitModel>();
  tab->setupFitPropertyBrowser<MultiFunctionTemplateBrowser, MultiFunctionTemplatePresenter,
                               MultiFunctionTemplateModel>(ConvFit::HIDDEN_PROPS, true);
  tab->setupFitDataView<ConvFitDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<ConvFitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeFqFitTab(int const index) const {
  auto tab = new IndirectDataAnalysisTab(FqFit::TAB_NAME, FqFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<FqFitModel>();
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, FqFunctionModel>(
      FqFit::HIDDEN_PROPS);
  tab->setupFitDataView<FqFitDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<FqFitDataPresenter>();
  tab->subscribeFitBrowserToDataPresenter();
  tab->setupPlotView(FqFit::X_BOUNDS);
  return tab;
}

} // namespace MantidQt::CustomInterfaces::IDA