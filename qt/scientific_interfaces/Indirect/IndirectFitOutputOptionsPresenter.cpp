// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsPresenter.h"

#include "IndirectFitOutputOptionsModel.h"
#include "IndirectFitOutputOptionsView.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

// IndirectFitOutputOptionsPresenter::IndirectFitOutputOptionsPresenter(
//    std::unique_ptr<IndirectFitAnalysisTab> tab,
//    IndirectFitOutputOptionsView *view)
//    : QObject(nullptr), m_view(view) {
//
//  m_model = Mantid::Kernel::make_unique<IndirectFitOutputOptionsModel>(tab);
//
//  connect(m_view.get(), SIGNAL(plotClicked()), this, SLOT(plotResult()));
//  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveResult()));
//}

IndirectFitOutputOptionsPresenter::IndirectFitOutputOptionsPresenter(
    IndirectFitOutputOptionsView *view)
    : QObject(nullptr), m_view(view) {

  m_model = std::make_unique<IndirectFitOutputOptionsModel>();

  connect(m_view.get(), SIGNAL(plotClicked()), this, SLOT(plotResult()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveResult()));
}

IndirectFitOutputOptionsPresenter::~IndirectFitOutputOptionsPresenter() {}

void IndirectFitOutputOptionsPresenter::setPlotWorkspace(
    WorkspaceGroup_sptr workspace) {
  m_model->setActivePlotWorkspace(workspace);
}

void IndirectFitOutputOptionsPresenter::setPlotParameters(
    std::vector<std::string> const &parameterNames) {
  m_view->clearPlotParameters();
  if (!parameterNames.empty()) {
    m_view->setAvailablePlotParameters(parameterNames);
    m_view->setSelectedParameterIndex(0);
  }
}

void IndirectFitOutputOptionsPresenter::setPlotEnabled(bool enable) {
  m_view->setPlotEnabled(enable);
}

void IndirectFitOutputOptionsPresenter::setSaveEnabled(bool enable) {
  m_view->setSaveEnabled(enable);
}

void IndirectFitOutputOptionsPresenter::plotResult() {
  m_view->setAsPlotting(true);
  try {
    m_model->plotResult(m_view->getPlotType());
  } catch (std::runtime_error const &ex) {
    displayWarning(ex.what());
  }
  m_view->setAsPlotting(false);
}

void IndirectFitOutputOptionsPresenter::displayWarning(
    std::string const &message) {
  m_view->displayWarning(message);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
