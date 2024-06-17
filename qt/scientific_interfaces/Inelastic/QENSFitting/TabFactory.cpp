// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "TabFactory.h"

#include "ConvolutionDataPresenter.h"
#include "FitDataPresenter.h"
#include "FitTab.h"
#include "FitTabConstants.h"
#include "FunctionBrowser/ConvFunctionTemplateModel.h"
#include "FunctionBrowser/FqFunctionModel.h"
#include "FunctionBrowser/IqtFunctionTemplateModel.h"
#include "FunctionBrowser/MSDFunctionModel.h"
#include "FunctionBrowser/MultiFunctionTemplatePresenter.h"
#include "FunctionBrowser/MultiFunctionTemplateView.h"
#include "FunctionBrowser/SingleFunctionTemplatePresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateView.h"
#include "FunctionQDataPresenter.h"
#include "FunctionQModel.h"
#include "IqtFitModel.h"
#include "MSDModel.h"

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

FitTab *TabFactory::makeMSDTab(int const index) const {
  auto tab = new FitTab(m_tabWidget->widget(index), MSD::TAB_NAME);
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, MSDFunctionModel>(
      MSD::HIDDEN_PROPS);
  tab->setupFittingPresenter<MSDModel>();
  tab->setupFitDataView<FitDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<FitDataPresenter>();
  tab->setupPlotView();
  return tab;
}

FitTab *TabFactory::makeIqtTab(int const index) const {
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

FitTab *TabFactory::makeConvolutionTab(int const index) const {
  auto tab = new FitTab(m_tabWidget->widget(index), Convolution::TAB_NAME);
  auto browserCustomizations = packBrowserCustomizations(Convolution::templateSubTypes());
  tab->setupFitPropertyBrowser<MultiFunctionTemplateView, MultiFunctionTemplatePresenter, ConvFunctionTemplateModel>(
      Convolution::HIDDEN_PROPS, true, std::move(browserCustomizations));
  tab->setupFittingPresenter<ConvolutionModel>();
  tab->setupFitDataView<ConvolutionDataView>();
  tab->setupOutputOptionsPresenter(true);
  tab->setUpFitDataPresenter<ConvolutionDataPresenter>();
  tab->setupPlotView();
  return tab;
}

FitTab *TabFactory::makeFunctionQTab(int const index) const {
  auto tab = new FitTab(m_tabWidget->widget(index), FunctionQ::TAB_NAME);
  tab->setupFitPropertyBrowser<SingleFunctionTemplateView, SingleFunctionTemplatePresenter, FqFunctionModel>(
      FunctionQ::HIDDEN_PROPS);
  tab->setupFittingPresenter<FunctionQModel>();
  tab->setupFitDataView<FunctionQDataView>();
  tab->setupOutputOptionsPresenter();
  tab->setUpFitDataPresenter<FunctionQDataPresenter>();
  tab->setupPlotView(FunctionQ::X_BOUNDS);
  return tab;
}

} // namespace MantidQt::CustomInterfaces::Inelastic