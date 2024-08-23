// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <QColor>
#include <QWidget>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

class Axes;
class FigureCanvasQt;

} // namespace MplCpp
} // namespace Widgets

namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_PLOTTING ContourPreviewPlot : public QWidget, public AnalysisDataServiceObserver {
  Q_OBJECT

public:
  ContourPreviewPlot(QWidget *parent = nullptr, bool observeADS = true);

  void watchADS(bool on);

  void setCanvasColour(QColor const &colour);

  void setWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace);
  void clearPlot();

  std::tuple<double, double> getAxisRange(AxisID axisID) const;

private:
  void createLayout();

  void replaceHandle(const std::string &wsName, const Workspace_sptr &workspace) override;
  void deleteHandle(const std::string &wsName, const Workspace_sptr &workspace) override;

  /// Canvas objects
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
};

} // namespace MantidWidgets
} // namespace MantidQt
