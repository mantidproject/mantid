#ifndef ONECURVEPLOT_H_
#define ONECURVEPLOT_H_

#include <QList>
#include <QMap>
#include <QStringList>
#include <qwt_plot.h>
#include <qwt_plot_item.h>

class QwtPlotCurve;
class QwtPlotZoomer;

namespace MantidQt {
namespace MantidWidgets {
class PeakLabel;
class PeakMarker2D;

/**
 * Implements a simple widget for plotting a single curve.
 * Allows to keep more than one curve.
 */
class OneCurvePlot : public QwtPlot {
  Q_OBJECT
public:
  explicit OneCurvePlot(QWidget *parent);
  ~OneCurvePlot() override;
  void setData(const double *x, const double *y, int dataSize,
               const std::string &xUnits = "");
  void setLabel(const QString &label);
  QString label() const { return m_label; }
  void setYAxisLabelRotation(double degrees);
  void addPeakLabel(const PeakMarker2D *);
  void clearPeakLabels();
  bool hasCurve() const;
  void store();
  bool hasStored() const;
  QStringList getLabels() const;
  void removeCurve(const QString &label);
  QColor getCurveColor(const QString &label) const;
  void recalcXAxisDivs();
  void recalcYAxisDivs();
  bool isYLogScale() const;
  const std::string &getXUnits() const { return m_xUnits; }
public slots:
  void setXScale(double from, double to);
  void setYScale(double from, double to);
  void clearCurve();
  void recalcAxisDivs();
  void setYLogScale();
  void setYLinearScale();
  void clearAll();
signals:
  void showContextMenu();
  void clickedAt(double, double);

protected:
  void resizeEvent(QResizeEvent *e) override;
  void contextMenuEvent(QContextMenuEvent *e) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;

private:
  QwtPlotCurve *m_curve;
  QString m_label;         ///< label to identify stored curve
  QwtPlotZoomer *m_zoomer; ///< does zooming
  int m_x0;                ///< save x coord of last left mouse click
  int m_y0;                ///< save y coord of last left mouse click
  QList<PeakLabel *> m_peakLabels;
  QMap<QString, QwtPlotCurve *> m_stored; ///< stored curves
  QList<QColor> m_colors;                 ///< colors for stored curves
  int m_colorIndex;
  std::string m_xUnits;
};

class PeakLabel : public QwtPlotItem {
public:
  PeakLabel(const PeakMarker2D *m, const OneCurvePlot *plot)
      : m_marker(m), m_plot(plot) {}
  void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &canvasRect) const override;

private:
  const PeakMarker2D *m_marker;
  const OneCurvePlot *m_plot;
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif /*ONECURVEPLOT_H_*/
