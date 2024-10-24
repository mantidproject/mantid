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
#include "MantidQtWidgets/Plotting/PlotWidget/PlotModel.h"

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_PLOTTING PlotPresenter {
public:
  PlotPresenter(IPlotView *view, std::unique_ptr<PlotModel> model = nullptr);
  virtual ~PlotPresenter() = default;

  virtual void clearModel();

  virtual void setSpectrum(const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex);

  virtual void setScaleLinear(const AxisID axisID);
  virtual void setScaleLog(const AxisID axisID);

  virtual void setPlotErrorBars(const bool plotErrorBars);

  virtual void plot();

private:
  IPlotView *m_view{nullptr};
  std::unique_ptr<PlotModel> m_model{nullptr};
};
} // namespace MantidQt::MantidWidgets
