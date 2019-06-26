// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MiniPlotQwt_H_
#define MiniPlotQwt_H_

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
class MiniPlotQwt : public QwtPlot {
  Q_OBJECT
public:
  explicit MiniPlotQwt(QWidget *parent);
  ~MiniPlotQwt() override;
  void setXLabel(QString xunit);
  void setData(std::vector<double> x, std::vector<double> y, QString xunit,
               QString curveLabel);
  QString label() const { return m_label; }
  void setYAxisLabelRotation(double degrees);
  void addPeakLabel(const PeakMarker2D * /*marker*/);
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
  QString getXUnits() const { return m_xUnits; }
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
  void clickedAt(double /*_t1*/, double /*_t2*/);

protected:
  void resizeEvent(QResizeEvent *e) override;
  void contextMenuEvent(QContextMenuEvent *e) override;
  void mousePressEvent(QMouseEvent * /*unused*/) override;
  void mouseReleaseEvent(QMouseEvent * /*unused*/) override;

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
  QString m_xUnits;
};

class PeakLabel : public QwtPlotItem {
public:
  PeakLabel(const PeakMarker2D *m, const MiniPlotQwt *plot)
      : m_marker(m), m_plot(plot) {}
  void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &canvasRect) const override;

private:
  const PeakMarker2D *m_marker;
  const MiniPlotQwt *m_plot;
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MiniPlotQwt_H_*/
