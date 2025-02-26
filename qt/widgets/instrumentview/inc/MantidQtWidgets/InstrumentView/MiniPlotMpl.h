// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/DllOption.h"
#include "MantidQtWidgets/MplCpp/Cycler.h"
#include "MantidQtWidgets/MplCpp/Line2D.h"
#include "MantidQtWidgets/MplCpp/PanZoomTool.h"
#include <QWidget>
#include <list>

class QPushButton;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class FigureCanvasQt;
}
} // namespace Widgets
namespace MantidWidgets {
class PeakMarker2D;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW MiniPlotMpl : public QWidget {
  Q_OBJECT
public:
  explicit MiniPlotMpl(QWidget *parent = nullptr);

  void setData(std::vector<double> x, std::vector<double> y, QString xunit, QString curveLabel);
  void setXLabel(QString xunit);
  QString label() const { return m_activeCurveLabel; }
  void addPeakLabel(const PeakMarker2D *peakMarker);
  void clearPeakLabels();
  bool hasCurve() const;
  void store();
  bool hasStored() const;
  QStringList getLabels() const { return m_storedCurveLabels; }
  void removeCurve(const QString &label);
  QColor getCurveColor(const QString &label) const;
  bool isYLogScale() const;
  void replot();

  // cppcheck-suppress unknownMacro
public slots:
  void clearCurve();
  void setYLogScale();
  void setYLinearScale();
  void clearAll();
  void zoomOutOnPlot();

signals:
  void showContextMenu();
  void clickedAt(double /*_t1*/, double /*_t2*/);

protected:
  bool eventFilter(QObject *watched, QEvent *evt) override;

private:
  bool handleMousePressEvent(QMouseEvent *evt);
  bool handleMouseReleaseEvent(QMouseEvent *evt);

  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  QPushButton *m_homeBtn;
  std::list<Widgets::MplCpp::Line2D> m_lines;
  std::list<Widgets::MplCpp::Artist> m_peakLabels;
  Widgets::MplCpp::Cycler m_colorCycler;
  QString m_xunit;
  QString m_activeCurveLabel;
  QStringList m_storedCurveLabels;
  Widgets::MplCpp::PanZoomTool m_zoomTool;
  QPoint m_mousePressPt;
};
} // namespace MantidWidgets
} // namespace MantidQt
