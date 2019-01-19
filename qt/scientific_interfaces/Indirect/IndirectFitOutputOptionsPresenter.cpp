// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsPresenter.h"

#include "IndirectFitAnalysisTab.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsPresenter::IndirectFitOutputOptionsPresenter(
    IndirectFitOutputOptionsView *view)
    : QObject(nullptr),
      m_model(std::make_unique<IndirectFitOutputOptionsModel>()), m_view(view) {
  connect(m_view.get(), SIGNAL(plotClicked()), this, SLOT(plotResult()));
  connect(m_view.get(), SIGNAL(plotClicked()), this, SIGNAL(plotSpectra()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveResult()));
}

IndirectFitOutputOptionsPresenter::~IndirectFitOutputOptionsPresenter() {}

void IndirectFitOutputOptionsPresenter::setPlotWorkspace(
    WorkspaceGroup_sptr workspace) {
  m_model->setActivePlotWorkspace(workspace);
}

void IndirectFitOutputOptionsPresenter::setPlotParameters(
    std::vector<std::string> const &parameterNames) {
  m_view->clearPlotTypes();
  if (!parameterNames.empty()) {
    m_view->setAvailablePlotTypes(
        m_model->formatParameterNames(parameterNames));
    m_view->setPlotTypeIndex(0);
  }
}

void IndirectFitOutputOptionsPresenter::plotResult() {
  setPlotting(true);
  try {
    m_model->plotResult(m_view->getSelectedPlotType());
  } catch (std::runtime_error const &ex) {
    displayWarning(ex.what());
  }
}

void IndirectFitOutputOptionsPresenter::saveResult() {
  setSaving(true);
  try {
    m_model->saveResult();
  } catch (std::runtime_error const &ex) {
    displayWarning(ex.what());
  }
  setSaving(false);
}

void IndirectFitOutputOptionsPresenter::setPlotting(bool plotting) {
  m_view->setPlotText(plotting ? "Plotting..." : "Plot");
  setPlotEnabled(!plotting);
  setSaveEnabled(!plotting);
}

void IndirectFitOutputOptionsPresenter::setSaving(bool saving) {
  m_view->setSaveText(saving ? "Saving..." : "Save Result");
  setPlotEnabled(!saving);
  setSaveEnabled(!saving);
}

void IndirectFitOutputOptionsPresenter::setPlotEnabled(bool enable) {
  m_view->setPlotEnabled(enable && m_model->plotWorkspaceIsPlottable());
}

void IndirectFitOutputOptionsPresenter::setSaveEnabled(bool enable) {
  m_view->setSaveEnabled(enable);
}

void IndirectFitOutputOptionsPresenter::clearSpectraToPlot() {
  m_model->clearSpectraToPlot();
}

std::vector<SpectrumToPlot>
IndirectFitOutputOptionsPresenter::getSpectraToPlot() const {
  return m_model->getSpectraToPlot();
}

void IndirectFitOutputOptionsPresenter::displayWarning(
    std::string const &message) {
  m_view->displayWarning(message);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
