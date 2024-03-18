// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DataAnalysisTabFactory.h"

#include "ConvFitDataPresenter.h"
#include "DataAnalysisTab.h"
#include "FitDataPresenter.h"
#include "FitTabConstants.h"
#include "FqFitDataPresenter.h"
#include "FqFitModel.h"
#include "FunctionBrowser/ConvFunctionTemplateModel.h"
#include "FunctionBrowser/FqFunctionModel.h"
#include "FunctionBrowser/IqtFunctionModel.h"
#include "FunctionBrowser/IqtFunctionTemplateView.h"
#include "FunctionBrowser/IqtTemplatePresenter.h"
#include "FunctionBrowser/MSDFunctionModel.h"
#include "FunctionBrowser/MultiFunctionTemplatePresenter.h"
#include "FunctionBrowser/MultiFunctionTemplateView.h"
#include "FunctionBrowser/SingleFunctionTemplatePresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateView.h"
#include "IqtFitModel.h"
#include "MSDFitModel.h"

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

TemplateBrowserCustomizations packBrowserCustomizations(std::unique_ptr<TemplateSubTypes> subTypes) {
  auto browserCustomizations = TemplateBrowserCustomizations();
  browserCustomizations.templateSubTypes = std::move(subTypes);
  return browserCustomizations;
}

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

DataAnalysisTabFactory::DataAnalysisTabFactory(QTabWidget *tabWidget) : m_tabWidget(tabWidget) {}

DataAnalysisTab *DataAnalysisTabFactory::makeMSDFitTab(int const index) const {
  auto tab = new DataAnalysisTab(MSDFit::TAB_NAME, MSDFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<MSDFitModel>();
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, MSDFunctionModel>(
      MSDFit::HIDDEN_PROPS);
  tab->setupFitDataView<FitDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<FitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

DataAnalysisTab *DataAnalysisTabFactory::makeIqtFitTab(int const index) const {
  auto tab = new DataAnalysisTab(IqtFit::TAB_NAME, IqtFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<IqtFitModel>();
  tab->setupFitPropertyBrowser<IqtFunctionTemplateView, IqtTemplatePresenter, IqtFunctionModel>(IqtFit::HIDDEN_PROPS);
  tab->setupFitDataView<FitDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<FitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

DataAnalysisTab *DataAnalysisTabFactory::makeConvFitTab(int const index) const {
  auto tab = new DataAnalysisTab(ConvFit::TAB_NAME, ConvFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<ConvFitModel>();
  auto browserCustomizations = packBrowserCustomizations(ConvFit::templateSubTypes());
  tab->setupFitPropertyBrowser<MultiFunctionTemplateView, MultiFunctionTemplatePresenter, ConvFunctionTemplateModel>(
      ConvFit::HIDDEN_PROPS, true, std::move(browserCustomizations));
  tab->setupFitDataView<ConvFitDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<ConvFitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

DataAnalysisTab *DataAnalysisTabFactory::makeFqFitTab(int const index) const {
  auto tab = new DataAnalysisTab(FqFit::TAB_NAME, FqFit::HAS_RESOLUTION, m_tabWidget->widget(index));
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