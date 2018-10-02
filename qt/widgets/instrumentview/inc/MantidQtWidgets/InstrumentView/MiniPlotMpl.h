#ifndef MANTIDQTWIDGETS_INSTRUMENTVIEW_MINIPLOTMPL_H
#define MANTIDQTWIDGETS_INSTRUMENTVIEW_MINIPLOTMPL_H
/*
  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "MantidQtWidgets/InstrumentView/DllOption.h"
#include "MantidQtWidgets/MplCpp/Line2D.h"
#include <QWidget>

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

  void setData(std::vector<double> x, std::vector<double> y,
               QString xunit, QString curveLabel);
  QString label() const { return "LABEL"; }
  void setYAxisLabelRotation(double degrees) {}
  void addPeakLabel(const PeakMarker2D *) {}
  void clearPeakLabels() {}
  bool hasCurve() const { return false; }
  void store() {}
  bool hasStored() const { return false; }
  QStringList getLabels() const { return {}; }
  void removeCurve(const QString &label) {}
  QColor getCurveColor(const QString &label) const { return QColor(); }
  void recalcXAxisDivs() {}
  void recalcYAxisDivs() {}
  bool isYLogScale() const { return false; }
  void replot();
public slots:
  void setXScale(double from, double to) {}
  void setYScale(double from, double to) {}
  void clearCurve();
  void recalcAxisDivs() {}
  void setYLogScale() {}
  void setYLinearScale() {}
  void clearAll();
signals:
  void showContextMenu();
  void clickedAt(double, double);

private:
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  std::vector<Widgets::MplCpp::Line2D> m_lines;
  QString m_xunit;
  QString m_activeCurveLabel;

};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTWIDGETS_INSTRUMENTVIEW_MINIPLOTMPL_H
