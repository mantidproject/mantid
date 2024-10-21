// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Line2D.h"
#include "MantidQtWidgets/MplCpp/PanZoomTool.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "MantidQtWidgets/Plotting/SingleSelector.h"

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
class EXPORT_OPT_MANTIDQT_PLOTTING PreviewPlot : public QWidget, public AnalysisDataServiceObserver {
  Q_OBJECT

  Q_PROPERTY(QColor canvasColour READ canvasColour WRITE setCanvasColour)
  Q_PROPERTY(bool showLegend READ legendIsVisible WRITE showLegend)
  Q_PROPERTY(QStringList curveErrorBars READ linesWithErrors WRITE setLinesWithErrors)

public:
  PreviewPlot(QWidget *parent = nullptr, bool observeADS = true);

  void watchADS(bool on);

  Widgets::MplCpp::FigureCanvasQt *canvas() const;
  QPointF toDataCoords(const QPoint &point) const;

  void setTightLayout(QHash<QString, QVariant> const &args);

  void addSpectrum(const QString &lineLabel, const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex = 0,
                   const QColor &lineColour = QColor(),
                   const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());
  void addSpectrum(const QString &lineName, const QString &wsName, const size_t wsIndex = 0,
                   const QColor &lineColour = QColor(),
                   const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());
  void removeSpectrum(const QString &lineName);

  RangeSelector *addRangeSelector(const QString &name, RangeSelector::SelectType type = RangeSelector::XMINMAX);
  RangeSelector *getRangeSelector(const QString &name) const;

  SingleSelector *addSingleSelector(const QString &name, SingleSelector::SelectType type = SingleSelector::XSINGLE,
                                    double position = 0.0, PlotLineStyle style = PlotLineStyle::Dash);
  SingleSelector *getSingleSelector(const QString &name) const;

  void setSelectorActive(bool active);
  bool selectorActive() const;

  bool hasCurve(const QString &lineName) const;

  void setOverrideAxisLabel(AxisID const &axisID, char const *const label);
  void tickLabelFormat(const std::string &axis, const std::string &style, bool useOffset);
  void setAxisRange(const QPair<double, double> &range, AxisID axisID = AxisID::XBottom);
  std::tuple<double, double> getAxisRange(AxisID axisID = AxisID::XBottom);

  void allowRedraws(bool state);
  void replotData();

public slots:
  void clear();
  void resizeX();
  void resetView();
  void setCanvasColour(const QColor &colour);
  void setLinesWithErrors(const QStringList &labels);
  void setLinesWithoutErrors(const QStringList &labels);
  void showLegend(bool visible);
  void replot();

signals:
  void mouseDown(const QPoint &point);
  void mouseUp(const QPoint &point);
  void mouseMove(const QPoint &point);
  void mouseHovering(const QPoint &point);

  void redraw();
  void resetSelectorBounds();

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
  bool handleWindowResizeEvent();

  void showContextMenu(QMouseEvent *evt);

  void createLayout();
  void createActions();

  void replaceHandle(const std::string &wsName, const Workspace_sptr &ws) override;
  void deleteHandle(const std::string &wsName, const Workspace_sptr &ws) override;

  void regenerateLegend();
  void removeLegend();

  void switchPlotTool(QAction *selected);
  void setXScaleType(QAction *selected);
  void setYScaleType(QAction *selected);
  void setErrorBars(QAction *selected);
  void setScaleType(AxisID id, const QString &actionName);
  void toggleLegend(const bool checked);

  std::optional<char const *> overrideAxisLabel(AxisID const &axisID);
  void setAxisLabel(AxisID const &axisID, char const *const label);

  // Block redrawing from taking place
  bool m_allowRedraws = true;

  // Curve configuration
  struct PlotCurveConfiguration {
    Mantid::API::MatrixWorkspace_sptr ws;
    QString lineName;
    size_t wsIndex;
    QColor lineColour;
    QHash<QString, QVariant> plotKwargs;

    PlotCurveConfiguration(Mantid::API::MatrixWorkspace_sptr ws, QString lineName, size_t wsIndex, QColor lineColour,
                           QHash<QString, QVariant> plotKwargs)
        : ws(ws), lineName(lineName), wsIndex(wsIndex), lineColour(lineColour), plotKwargs(plotKwargs) {};
  };

  // Canvas objects
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  // Map a line label to the boolean indicating whether error bars are shown
  QHash<QString, bool> m_lines;
  // Map a line name to a plot configuration
  QMap<QString, QSharedPointer<PlotCurveConfiguration>> m_plottedLines;
  // Cache of line names which always have errors
  QHash<QString, bool> m_linesErrorsCache;
  // Map an axis to an override axis label
  QMap<AxisID, char const *> m_axisLabels;
  // Range selector widgets
  QMap<QString, RangeSelector *> m_rangeSelectors;
  // Single selector's
  QMap<QString, SingleSelector *> m_singleSelectors;
  // Whether or not a selector is currently being moved
  bool m_selectorActive;

  // Canvas tools
  Widgets::MplCpp::PanZoomTool m_panZoomTool;

  // Tick label style
  std::string m_axis;
  std::string m_style;
  bool m_useOffset;

  // Axis scales
  std::string m_xAxisScale;
  std::string m_yAxisScale;

  // Whether to redraw markers when a paint event occurs
  bool m_redrawOnPaint;

  // Context menu actions
  QActionGroup *m_contextPlotTools;
  QAction *m_contextResetView;
  QActionGroup *m_contextXScale, *m_contextYScale;
  QAction *m_contextLegend;
  QActionGroup *m_contextErrorBars;
};

} // namespace MantidWidgets
} // namespace MantidQt
