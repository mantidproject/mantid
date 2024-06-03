// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "TabFactory.h"

#include "ConvFitDataPresenter.h"
#include "FitDataPresenter.h"
#include "FitTab.h"
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

namespace {
using namespace MantidQt::CustomInterfaces::Inelastic;

TemplateBrowserCustomizations packBrowserCustomizations(std::unique_ptr<TemplateSubTypes> subTypes) {
  auto browserCustomizations = TemplateBrowserCustomizations();
  browserCustomizations.templateSubTypes = std::move(subTypes);
  return browserCustomizations;
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

TabFactory::TabFactory(QTabWidget *tabWidget) : m_tabWidget(tabWidget) {}

FitTab *TabFactory::makeMSDFitTab(int const index) const {
  auto tab = new FitTab(m_tabWidget->widget(index), MSDFit::TAB_NAME);
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, MSDFunctionModel>(
      MSDFit::HIDDEN_PROPS);
  tab->setupFittingPresenter<MSDFitModel>();
  tab->setupFitDataView<FitDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<FitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

FitTab *TabFactory::makeIqtFitTab(int const index) const {
  auto tab = new FitTab(m_tabWidget->widget(index), IqtFit::TAB_NAME);
  auto browserCustomizations = packBrowserCustomizations(IqtFit::templateSubTypes());
  tab->setupFitPropertyBrowser<MultiFunctionTemplateView, MultiFunctionTemplatePresenter, IqtFunctionTemplateModel>(
      IqtFit::HIDDEN_PROPS, false, std::move(browserCustomizations));
  tab->setupFittingPresenter<IqtFitModel>();
  tab->setupFitDataView<FitDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<FitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

FitTab *TabFactory::makeConvFitTab(int const index) const {
  auto tab = new FitTab(m_tabWidget->widget(index), ConvFit::TAB_NAME);
  auto browserCustomizations = packBrowserCustomizations(ConvFit::templateSubTypes());
  tab->setupFitPropertyBrowser<MultiFunctionTemplateView, MultiFunctionTemplatePresenter, ConvFunctionTemplateModel>(
      ConvFit::HIDDEN_PROPS, true, std::move(browserCustomizations));
  tab->setupFittingPresenter<ConvFitModel>();
  tab->setupFitDataView<ConvFitDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<ConvFitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

FitTab *TabFactory::makeFqFitTab(int const index) const {
  auto tab = new FitTab(m_tabWidget->widget(index), FqFit::TAB_NAME);
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, FqFunctionModel>(
      FqFit::HIDDEN_PROPS);
  tab->setupFittingPresenter<FqFitModel>();
  tab->setupFitDataView<FqFitDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<FqFitDataPresenter>();
  tab->setupPlotView(FqFit::X_BOUNDS);
  return tab;
}

} // namespace MantidQt::CustomInterfaces::Inelastic