// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/PlotWidget/PlotPresenter.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

using Mantid::API::MatrixWorkspace_sptr;

namespace MantidQt::MantidWidgets {
PlotPresenter::PlotPresenter(IPlotView *view) {}

void PlotPresenter::setScaleLinear(const AxisID axisID) {}

void PlotPresenter::setScaleLog(const AxisID axisID) {}
} // namespace MantidQt::MantidWidgets
