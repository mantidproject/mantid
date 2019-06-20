// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_
#define MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Line2D.h"
#include "MantidQtWidgets/MplCpp/PanZoomTool.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/Mpl/RangeSelector.h"

#include <Poco/NObserver.h>

#include <QHash>
#include <QPair>
#include <QVariant>
#include <QWidget>
#include <list>

class QAction;
class QActionGroup;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class Axes;
class FigureCanvasQt;
} // namespace MplCpp
} // namespace Widgets
namespace MantidWidgets {

/**
 * Displays several workpaces on a matplotlib figure
 */
class EXPORT_OPT_MANTIDQT_PLOTTING PreviewPlot : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QColor canvasColour READ canvasColour WRITE setCanvasColour)
  Q_PROPERTY(bool showLegend READ legendIsVisible WRITE showLegend)
  Q_PROPERTY(
      QStringList curveErrorBars READ linesWithErrors WRITE setLinesWithErrors)

public:
  PreviewPlot(QWidget *parent = nullptr, bool observeADS = true);
  ~PreviewPlot();

  void watchADS(bool on);

  Widgets::MplCpp::FigureCanvasQt *canvas() const;
  QPointF toDataCoords(const QPoint &point) const;

  void addSpectrum(
      const QString &lineLabel, const Mantid::API::MatrixWorkspace_sptr &ws,
      const size_t wsIndex = 0, const QColor &lineColour = QColor(),
      const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());
  void addSpectrum(
      const QString &lineName, const QString &wsName, const size_t wsIndex = 0,
      const QColor &lineColour = QColor(),
      const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());
  void removeSpectrum(const QString &lineName);

  RangeSelector *
  addRangeSelector(const QString &name,
                   RangeSelector::SelectType type = RangeSelector::XMINMAX);
  RangeSelector *getRangeSelector(const QString &name) const;

  void setAxisRange(const QPair<double, double> &range,
                    AxisID axisID = AxisID::XBottom);
  std::tuple<double, double> getAxisRange(AxisID axisID = AxisID::XBottom);

  void replot();

public slots:
  void clear();
  void resizeX();
  void resetView();
  void setCanvasColour(QColor colour);
  void setLinesWithErrors(QStringList labels);
  void showLegend(bool visible);

signals:
  void mouseDown(const QPoint &point);
  void mouseUp(const QPoint &point);
  void mouseMove(const QPoint &point);

  void redraw();

public:
  QColor canvasColour() const;
  bool legendIsVisible() const;
  QStringList linesWithErrors() const;

protected:
  bool eventFilter(QObject *watched, QEvent *evt) override;

private:
  bool handleMousePressEvent(QMouseEvent *evt);
  bool handleMouseReleaseEvent(QMouseEvent *evt);
  bool handleMouseMoveEvent(QMouseEvent *evt);

  void showContextMenu(QMouseEvent *evt);

  void createLayout();
  void createActions();

  void onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf);
  void
  onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf);

  void regenerateLegend();
  void removeLegend();

  void switchPlotTool(QAction *selected);
  void setXScaleType(QAction *selected);
  void setYScaleType(QAction *selected);
  void setScaleType(AxisID id, QString actionName);
  void toggleLegend(const bool checked);

  // Canvas objects
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  // Map a line label to the boolean indicating whether error bars are shown
  QHash<QString, bool> m_lines;
  // Range selector widgets
  QMap<QString, MantidQt::MantidWidgets::RangeSelector *> m_rangeSelectors;

  // Canvas tools
  Widgets::MplCpp::PanZoomTool m_panZoomTool;

  // Observers for ADS Notifications
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspacePreDeleteNotification>
      m_wsRemovedObserver;
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspaceBeforeReplaceNotification>
      m_wsReplacedObserver;

  // Context menu actions
  QActionGroup *m_contextPlotTools;
  QAction *m_contextResetView;
  QActionGroup *m_contextXScale, *m_contextYScale;
  QAction *m_contextLegend;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_
