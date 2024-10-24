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
  virtual ~PlotModel() = default;

  virtual std::vector<Mantid::API::MatrixWorkspace_sptr> getWorkspaces() const;
  virtual std::vector<int> getWorkspaceIndices() const;

  virtual void clear() noexcept;

  virtual void setSpectrum(const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex);

  virtual bool getPlotErrorBars() const { return m_plotErrorBars; }
  virtual void setPlotErrorBars(const bool plotErrorBars) { m_plotErrorBars = plotErrorBars; }

private:
  std::vector<Mantid::API::MatrixWorkspace_sptr> m_workspaces;
  std::vector<int> m_workspaceIndices;
  bool m_plotErrorBars{false};
};
} // namespace MantidQt::MantidWidgets
