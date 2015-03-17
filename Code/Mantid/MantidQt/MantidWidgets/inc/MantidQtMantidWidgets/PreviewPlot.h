#ifndef MANTIDQTMANTIDWIDGETS_PREVIEWPLOT_H_
#define MANTIDQTMANTIDWIDGETS_PREVIEWPLOT_H_

#include "ui_PreviewPlot.h"

#include "WidgetDllOption.h"
#include "MantidQtAPI/MantidWidget.h"

#include "MantidAPI/MatrixWorkspace.h"

#include <QActionGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>


namespace MantidQt
{
namespace MantidWidgets
{
  /**
  A widget to display several workspaces on a plot on a custom interface.

  Gives option to use pan and zoom options to navigate plot.

  @author Dan Nixon

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS PreviewPlot : public API::MantidWidget
  {
    Q_OBJECT

    Q_PROPERTY(QColor canvasColour READ canvasColour WRITE setCanvasColour)
    Q_PROPERTY(bool showLegend READ legendIsShown WRITE showLegend)

  public:
    PreviewPlot(QWidget *parent = NULL, bool init = true);
    virtual ~PreviewPlot();

    QColor canvasColour();
    void setCanvasColour(const QColor & colour);

    bool legendIsShown();

    void setAxisRange(QPair<double, double> range, int axisID = QwtPlot::xBottom);

    QPair<double, double> getCurveRange(const Mantid::API::MatrixWorkspace_sptr ws);
    QPair<double, double> getCurveRange(const QString & curveName);

    void addSpectrum(const QString & curveName, const Mantid::API::MatrixWorkspace_sptr ws, const size_t specIndex = 0, const QColor & curveColour = QColor());
    void addSpectrum(const QString & curveName, const QString & wsName, const size_t specIndex = 0, const QColor & curveColour = QColor());

    void removeSpectrum(const Mantid::API::MatrixWorkspace_sptr ws);
    void removeSpectrum(const QString & curveName);

    bool hasCurve(const QString & curveName);

    QString getAxisType(int axisID);

  signals:
    /// Signals that the plot should be refreshed
    void needToReplot();
    void needToHardReplot();
    /// Signals that the axis scale has been changed
    void axisScaleChanged();

  public slots:
    void showLegend(bool show);
    void togglePanTool(bool enabled);
    void toggleZoomTool(bool enabled);
    void resetView();
    void resizeX();
    void clear();
    void replot();
    void hardReplot();

  private:
    /// Holds information about a plot curve
    struct PlotCurveConfiguration
    {
      Mantid::API::MatrixWorkspace_sptr ws;
      QwtPlotCurve *curve;
      QLabel *label;
      QColor colour;
      size_t wsIndex;

      PlotCurveConfiguration():
        curve(NULL), label(NULL) {}
    };

    void handleRemoveEvent(Mantid::API::WorkspacePreDeleteNotification_ptr pNf);
    void handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);

    QwtPlotCurve * addCurve(Mantid::API::MatrixWorkspace_sptr ws, const size_t specIndex, const QColor & curveColour);
    void removeCurve(QwtPlotCurve *curve);

    QList<QAction *> addOptionsToMenus(QString menuName, QActionGroup *group, QStringList items, QString defaultItem);

    QStringList getCurvesForWorkspace(const Mantid::API::MatrixWorkspace_sptr ws);

  private slots:
    void showContextMenu(QPoint position);
    void handleViewToolSelect();
    void handleAxisTypeSelect();

  private:
    Ui::PreviewPlot m_uiForm;

    /// Poco Observers for ADS Notifications
    Poco::NObserver<PreviewPlot, Mantid::API::WorkspacePreDeleteNotification> m_removeObserver;
    Poco::NObserver<PreviewPlot, Mantid::API::WorkspaceAfterReplaceNotification> m_replaceObserver;

    /// If the widget was initialised
    bool m_init;

    /// The plot its self
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

  };

}
}

#endif //MANTIDQTMANTIDWIDGETS_PREVIEWPLOT_H_
