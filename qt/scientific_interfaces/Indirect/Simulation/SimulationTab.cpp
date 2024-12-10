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

void SimulationTab::setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

void SimulationTab::clearOutputPlotOptionsWorkspaces() { m_plotOptionsPresenter->clearWorkspaces(); }

void SimulationTab::enableLoadHistoryProperty(bool doLoadHistory) { setLoadHistory(doLoadHistory); }

} // namespace MantidQt::CustomInterfaces
