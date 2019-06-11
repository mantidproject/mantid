// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_QWT_PREVIEWPLOT_H_
#define MANTIDQTMANTIDWIDGETS_QWT_PREVIEWPLOT_H_

#include "ui_PreviewPlot.h"

#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/Qwt/ErrorCurve.h"
#include "MantidQtWidgets/Plotting/Qwt/RangeSelector.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <Poco/NObserver.h>

#include <QActionGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_symbol.h>

namespace MantidQt {
namespace MantidWidgets {
/**
A widget to display several workspaces on a plot on a custom interface.

Gives option to use pan and zoom options to navigate plot.

@author Dan Nixon
*/

// forward declaration
class DisplayCurveFit;

class EXPORT_OPT_MANTIDQT_PLOTTING PreviewPlot : public API::MantidWidget {
  Q_OBJECT

  Q_PROPERTY(QColor canvasColour READ canvasColour WRITE setCanvasColour)
  Q_PROPERTY(bool showLegend READ legendIsShown WRITE showLegend)
  Q_PROPERTY(QStringList curveErrorBars READ getShownErrorBars WRITE
                 setLinesWithErrors)

public:
  PreviewPlot(QWidget *parent = nullptr, bool init = true);
  ~PreviewPlot() override;
  void watchADS(bool on);

  QwtPlotCanvas *canvas() const;
  QwtPlot *getPlot() const;
  QColor canvasColour();
  void setCanvasColour(const QColor &colour);

  bool legendIsShown();
  QStringList getShownErrorBars();

  void setAxisRange(QPair<double, double> range,
                    AxisID axisID = AxisID::XBottom);

  QPair<double, double>
  getCurveRange(const Mantid::API::MatrixWorkspace_sptr ws);
  QPair<double, double> getCurveRange(const QString &curveName);

  void addSpectrum(
      const QString &curveName, const Mantid::API::MatrixWorkspace_sptr &ws,
      const size_t wsIndex = 0, const QColor &curveColour = QColor(),
      const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());
  void addSpectrum(
      const QString &curveName, const QString &wsName, const size_t wsIndex = 0,
      const QColor &curveColour = QColor(),
      const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());

  void removeSpectrum(const Mantid::API::MatrixWorkspace_sptr ws);
  void removeSpectrum(const QString &curveName);

  bool hasCurve(const QString &curveName);

  void setCurveStyle(const QString &curveName, const int style);
  void setCurveSymbol(const QString &curveName, const int symbol);

  RangeSelector *
  addRangeSelector(const QString &rsName,
                   RangeSelector::SelectType type = RangeSelector::XMINMAX);
  RangeSelector *getRangeSelector(const QString &rsName);
  void removeRangeSelector(const QString &rsName, bool del);

  bool hasRangeSelector(const QString &rsName);

  QString getAxisType(int axisID);

signals:
  /// Signals that the plot should be refreshed
  void needToReplot();
  void needToHardReplot();
  /// Signals that the axis scale has been changed
  void axisScaleChanged();
  /// Signals that workspace has been removed
  void workspaceRemoved(Mantid::API::MatrixWorkspace_sptr /*_t1*/);

public slots:
  void showLegend(bool show);
  void setLinesWithErrors(const QStringList &curveNames);
  void togglePanTool(bool enabled);
  void toggleZoomTool(bool enabled);
  void resetView();
  void resizeX();
  void clear();
  void replot();
  void hardReplot();

private:
  /// Holds information about a plot curve
  struct PlotCurveConfiguration {
    Mantid::API::MatrixWorkspace_sptr ws;
    QwtPlotCurve *curve;
    MantidQt::MantidWidgets::ErrorCurve *errorCurve;
    QAction *showErrorsAction;
    QLabel *label;
    QColor colour;
    size_t wsIndex;

    PlotCurveConfiguration()
        : curve(nullptr), errorCurve(nullptr), showErrorsAction(nullptr),
          label(nullptr), colour(), wsIndex(0) {}
  };

  void handleRemoveEvent(Mantid::API::WorkspacePreDeleteNotification_ptr pNf);
  void
  handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);

  void addCurve(PlotCurveConfiguration &curveConfig,
                Mantid::API::MatrixWorkspace_sptr ws, const size_t wsIndex,
                const QColor &curveColour);
  void removeCurve(QwtPlotItem *curve);

  QList<QAction *> addOptionsToMenus(QString menuName, QActionGroup *group,
                                     QStringList items, QString defaultItem);

  QStringList getCurvesForWorkspace(const Mantid::API::MatrixWorkspace_sptr ws);

private slots:
  void showContextMenu(QPoint position);
  void handleViewToolSelect();
  void handleAxisTypeSelect();
  void removeWorkspace(Mantid::API::MatrixWorkspace_sptr ws);

private:
  Ui::PreviewPlot m_uiForm;

  /// Range selector widget for mini plot
  QMap<QString, MantidQt::MantidWidgets::RangeSelector *> m_rangeSelectors;
  /// Cache of range selector visibility
  QMap<QString, bool> m_rsVisibility;

  /// Poco Observers for ADS Notifications
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspacePreDeleteNotification>
      m_removeObserver;
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspaceAfterReplaceNotification>
      m_replaceObserver;

  /// If the widget was initialised
  bool m_init;

  friend class RangeSelector;

  /// Map of curve key to plot info
  QMap<QString, PlotCurveConfiguration> m_curves;

  /// Plot manipulation tools
  QwtPlotMagnifier *m_magnifyTool;
  QwtPlotPanner *m_panTool;
  QwtPlotZoomer *m_zoomTool;

  /// Context menu items
  QMenu *m_contextMenu;
  QActionGroup *m_plotToolGroup;
  QActionGroup *m_xAxisTypeGroup;
  QActionGroup *m_yAxisTypeGroup;

  /// Menu action for showing/hiding plot legend
  QAction *m_showLegendAction;

  /// Menu group for error bar show/hide
  QAction *m_showErrorsMenuAction;
  QMenu *m_showErrorsMenu;

  /// Cache of error bar options
  // Persists error bar options when curves of same name are removed and
  // readded
  QMap<QString, bool> m_errorBarOptionCache;

  QwtPlotCurve::CurveStyle m_curveStyle;
  QwtSymbol::Style m_curveSymbol;

  friend class DisplayCurveFit;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_QWT_PREVIEWPLOT_H_
