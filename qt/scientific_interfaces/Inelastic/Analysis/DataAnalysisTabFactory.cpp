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
#include "FunctionBrowser/ConvTemplateBrowser.h"
#include "FunctionBrowser/FqTemplateBrowser.h"
#include "FunctionBrowser/IqtTemplateBrowser.h"
#include "FunctionBrowser/MSDTemplateBrowser.h"
#include "IndirectDataAnalysisTab.h"
#include "IndirectFitDataPresenter.h"
#include "IqtFitModel.h"
#include "MSDFitModel.h"

namespace MantidQt::CustomInterfaces::IDA {

DataAnalysisTabFactory::DataAnalysisTabFactory(QTabWidget *tabWidget) : m_tabWidget(tabWidget) {}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeMSDFitTab(int const index) const {
  auto tab =
      new IndirectDataAnalysisTab(new MSDFitModel, new MSDTemplateBrowser, new IndirectFitDataView, MSDFit::TAB_NAME,
                                  MSDFit::HAS_RESOLUTION, MSDFit::HIDDEN_PROPS, m_tabWidget->widget(index));
  tab->setUpTab<IndirectFitDataPresenter>();
  return tab;
}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeIqtFitTab(int const index) const {
  auto tab =
      new IndirectDataAnalysisTab(new IqtFitModel, new IqtTemplateBrowser, new IndirectFitDataView, IqtFit::TAB_NAME,
                                  IqtFit::HAS_RESOLUTION, IqtFit::HIDDEN_PROPS, m_tabWidget->widget(index));
  tab->setUpTab<IndirectFitDataPresenter>();
  return tab;
}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeConvFitTab(int const index) const {
  auto tab =
      new IndirectDataAnalysisTab(new ConvFitModel, new ConvTemplateBrowser, new ConvFitDataView, ConvFit::TAB_NAME,
                                  ConvFit::HAS_RESOLUTION, ConvFit::HIDDEN_PROPS, m_tabWidget->widget(index));
  tab->setUpTab<ConvFitDataPresenter>();
  tab->setConvolveMembers(true);
  return tab;
}

IndirectDataAnalysisTab *DataAnalysisTabFactory::makeFqFitTab(int const index) const {
  auto tab = new IndirectDataAnalysisTab(new FqFitModel, new FqTemplateBrowser, new FqFitDataView, FqFit::TAB_NAME,
                                         FqFit::HAS_RESOLUTION, FqFit::HIDDEN_PROPS, m_tabWidget->widget(index));
  tab->setUpTab<FqFitDataPresenter>(FqFit::X_BOUNDS);
  return tab;
}

} // namespace MantidQt::CustomInterfaces::IDA