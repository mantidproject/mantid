// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSimulationTab.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

IndirectSimulationTab::IndirectSimulationTab(QWidget *parent)
    : IndirectTab(parent) {}

IndirectSimulationTab::~IndirectSimulationTab() {}

void IndirectSimulationTab::setOutputPlotOptionsPresenter(
    std::unique_ptr<IndirectPlotOptionsPresenter> presenter) {
  m_plotOptionsPresenter = std::move(presenter);
}

void IndirectSimulationTab::setOutputPlotOptionsWorkspaces(
    std::vector<std::string> const &outputWorkspaces) {
  m_plotOptionsPresenter->setWorkspaces(outputWorkspaces);
}

void IndirectSimulationTab::clearOutputPlotOptionsWorkspaces() {
  m_plotOptionsPresenter->clearWorkspaces();
}

} // namespace CustomInterfaces
} // namespace MantidQt
