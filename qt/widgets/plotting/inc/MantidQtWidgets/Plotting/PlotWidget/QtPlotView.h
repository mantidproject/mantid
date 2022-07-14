// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/PlotWidget/IPlotView.h"

#include <QWidget>
#include <vector>

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_PLOTTING QtPlotView : public QWidget, public IPlotView {
  Q_OBJECT
public:
  enum class AxisScale { LINEAR, LOG };
  QtPlotView(QWidget *parent = nullptr);

  void setScaleLinear(const AxisID axisID) override;
  void setScaleLog(const AxisID axisID) override;

  void plot(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces,
            const std::vector<int> &workspaceIndices) override;

private:
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  std::vector<Mantid::API::MatrixWorkspace_sptr> m_workspaces;

  std::vector<int> m_workspaceIndices;

  QHash<QString, QVariant> m_axisProperties;
  Widgets::MplCpp::Figure createFigure();
  void createLayout();
};
} // namespace MantidQt::MantidWidgets