// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "TabFactory.h"

#include "ConvFitDataPresenter.h"
#include "FitDataPresenter.h"
#include "FitTabConstants.h"
#include "FqFitDataPresenter.h"
#include "FqFitModel.h"
#include "FunctionBrowser/ConvFunctionTemplateModel.h"
#include "FunctionBrowser/FqFunctionModel.h"
#include "FunctionBrowser/IqtFunctionTemplateModel.h"
#include "FunctionBrowser/MSDFunctionModel.h"
#include "FunctionBrowser/MultiFunctionTemplatePresenter.h"
#include "FunctionBrowser/MultiFunctionTemplateView.h"
#include "FunctionBrowser/SingleFunctionTemplatePresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateView.h"
#include "IqtFitModel.h"
#include "MSDFitModel.h"
#include "Tab.h"

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

TemplateBrowserCustomizations packBrowserCustomizations(std::unique_ptr<TemplateSubTypes> subTypes) {
  auto browserCustomizations = TemplateBrowserCustomizations();
  browserCustomizations.templateSubTypes = std::move(subTypes);
  return browserCustomizations;
}

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

TabFactory::TabFactory(QTabWidget *tabWidget) : m_tabWidget(tabWidget) {}

Tab *TabFactory::makeMSDFitTab(int const index) const {
  auto tab = new Tab(MSDFit::TAB_NAME, MSDFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<MSDFitModel>();
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, MSDFunctionModel>(
      MSDFit::HIDDEN_PROPS);
  tab->setupFitDataView<FitDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<FitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

Tab *TabFactory::makeIqtFitTab(int const index) const {
  auto tab = new Tab(IqtFit::TAB_NAME, IqtFit::HAS_RESOLUTION, m_tabWidget->widget(index));
  tab->setupFittingModel<IqtFitModel>();
  auto browserCustomizations = packBrowserCustomizations(IqtFit::templateSubTypes());
  tab->setupFitPropertyBrowser<MultiFunctionTemplateView, MultiFunctionTemplatePresenter, IqtFunctionTemplateModel>(
      IqtFit::HIDDEN_PROPS, false, std::move(browserCustomizations));
  tab->setupFitDataView<FitDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<FitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

Tab *TabFactory::makeConvFitTab(int const index) const {
  auto tab = new Tab(ConvFit::TAB_NAME, ConvFit::HAS_RESOLUTION, m_tabWidget->widget(index));
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

Tab *TabFactory::makeFqFitTab(int const index) const {
  auto tab = new Tab(FqFit::TAB_NAME, FqFit::HAS_RESOLUTION, m_tabWidget->widget(index));
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