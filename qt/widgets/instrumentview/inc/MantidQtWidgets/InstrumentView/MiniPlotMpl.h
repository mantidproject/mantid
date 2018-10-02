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
#include "MantidQtWidgets/MplCpp/Cycler.h"
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

  void setData(std::vector<double> x, std::vector<double> y, QString xunit,
               QString curveLabel);
  QString label() const { return m_activeCurveLabel; }
  void addPeakLabel(const PeakMarker2D *) {}
  void clearPeakLabels() {}
  bool hasCurve() const;
  void store();
  bool hasStored() const;
  QStringList getLabels() const { return m_storedCurveLabels; }
  void removeCurve(const QString &label);
  QColor getCurveColor(const QString &label) const;
  void recalcXAxisDivs() {}
  void recalcYAxisDivs() {}
  bool isYLogScale() const { return false; }
  void replot();
public slots:
  void clearCurve();
  void recalcAxisDivs() {}
  void setYLogScale() {}
  void setYLinearScale() {}
  void clearAll();
signals:
  void showContextMenu();
  void clickedAt(double, double);

protected:
  void contextMenuEvent(QContextMenuEvent *e) override;
  bool eventFilter(QObject *watched, QEvent *evt) override;
  void mousePressEvent(QMouseEvent *) override;

private: // data
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  std::vector<Widgets::MplCpp::Line2D> m_lines;
  Widgets::MplCpp::Cycler m_colorCycler;
  QString m_xunit;
  QString m_activeCurveLabel;
  QStringList m_storedCurveLabels;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTWIDGETS_INSTRUMENTVIEW_MINIPLOTMPL_H
