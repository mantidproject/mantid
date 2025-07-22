// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/PlotWidget/IPlotView.h"

#include <QWidget>
#include <vector>

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_PLOTTING QtPlotView : public QWidget, public IPlotView {
  Q_OBJECT
public:
  QtPlotView(QWidget *parent = nullptr);

  void setScaleLinear(const AxisID axisID) override;
  void setScaleLog(const AxisID axisID) override;
  void setScaleSymLog(const AxisID axisID, const double linthresh) override;
  void setScale(const AxisID axisID, const std::string &scale,
                const std::unordered_map<QString, QVariant> &additionalProperties = {}) override;
  void plot(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces, const std::vector<int> &workspaceIndices,
            const bool plotErrorBars) override;
  void setAxisLimit(const AxisID axisID, const double axMin, const double axMax) override;

private:
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  QHash<QString, QVariant> m_axisProperties;

  void createLayout();
};
} // namespace MantidQt::MantidWidgets
