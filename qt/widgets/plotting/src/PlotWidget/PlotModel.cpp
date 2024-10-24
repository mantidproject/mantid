// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/PlotWidget/PlotModel.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <vector>

using namespace Mantid::API;

namespace MantidQt::MantidWidgets {

std::vector<Mantid::API::MatrixWorkspace_sptr> PlotModel::getWorkspaces() const { return m_workspaces; }

std::vector<int> PlotModel::getWorkspaceIndices() const { return m_workspaceIndices; }

void PlotModel::clear() noexcept {
  m_workspaces.clear();
  m_workspaceIndices.clear();
}

void PlotModel::setSpectrum(const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex) {
  m_workspaces = std::vector<MatrixWorkspace_sptr>{ws};
  m_workspaceIndices = std::vector<int>{static_cast<int>(wsIndex)};
}
} // namespace MantidQt::MantidWidgets
