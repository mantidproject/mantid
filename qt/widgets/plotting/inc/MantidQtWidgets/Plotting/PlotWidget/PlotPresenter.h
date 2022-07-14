// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/PlotWidget/IPlotView.h"

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_PLOTTING PlotPresenter {
public:
  PlotPresenter(IPlotView *view);
  virtual ~PlotPresenter() = default;

  virtual void setSpectrum(const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex);

  virtual void setScaleLinear(const AxisID axisID);
  virtual void setScaleLog(const AxisID axisID);

private:
  IPlotView *m_view = nullptr;
};
} // namespace MantidQt::MantidWidgets