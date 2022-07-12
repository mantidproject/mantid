// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <vector>

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_PLOTTING PlotModel {
public:
  std::vector<Mantid::API::MatrixWorkspace_sptr> getWorkspaces() const;
  std::vector<int> getWorkspaceIndices() const;

  void setSpectrum(const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex);

private:
  std::vector<Mantid::API::MatrixWorkspace_sptr> m_workspaces;
  std::vector<int> m_workspaceIndices;
};
} // namespace MantidQt::MantidWidgets