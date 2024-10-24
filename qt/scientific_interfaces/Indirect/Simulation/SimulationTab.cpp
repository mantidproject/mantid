// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SimulationTab.h"

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

SimulationTab::SimulationTab(QWidget *parent) : InelasticTab(parent) {}

SimulationTab::~SimulationTab() = default;

void SimulationTab::setOutputPlotOptionsPresenter(
    IOutputPlotOptionsView *view, PlotWidget const &plotType, std::string const &fixedIndices,
    std::optional<std::map<std::string, std::string>> const &availableActions) {
  auto OutputOptionsModel =
      std::make_unique<OutputPlotOptionsModel>(std::make_unique<ExternalPlotter>(), availableActions);
  m_plotOptionsPresenter =
      std::make_unique<OutputPlotOptionsPresenter>(view, std::move(OutputOptionsModel), plotType, fixedIndices);
}

void SimulationTab::setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

void SimulationTab::clearOutputPlotOptionsWorkspaces() { m_plotOptionsPresenter->clearWorkspaces(); }

void SimulationTab::enableLoadHistoryProperty(bool doLoadHistory) { setLoadHistory(doLoadHistory); }

} // namespace MantidQt::CustomInterfaces
