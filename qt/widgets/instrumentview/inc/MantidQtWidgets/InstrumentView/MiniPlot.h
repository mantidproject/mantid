#ifndef MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOT_H
#define MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOT_H
/*
 Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidQtWidgets/MplCpp/MplFigureCanvas.h"
#include <QActionGroup>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

using MantidQt::Widgets::MplCpp::PythonObject;

/// Encapsulate the data required for a plot. Error data is optional.
struct MiniPlotCurveData {
  QString xunit;
  QString label;
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> e;
};

/**
 * Extends an MplFigureCanvas to provide a the mini plot functionality
 * embedded within the instrument view pick tab.
 */
class MiniPlot : public Widgets::MplCpp::MplFigureCanvas {
  Q_OBJECT

public:
  MiniPlot(QWidget *parent = nullptr);

  bool hasActiveCurve() const;
  bool hasStoredCurves() const;
  QString activeCurveLabel() const { return m_activeCurveLabel; }
  QStringList storedCurveLabels() const { return m_storedCurveLabels; }
  QString getXUnits() const { return m_xunit; }
  std::tuple<double, double> getYLimits() const;

  void update();
  void setActiveCurve(MiniPlotCurveData data);
  void removeActiveCurve();
  void removeCurve(QString label);
  void addPeakLabel(double x, double y, QString label);

public slots:
  void storeCurve();
  void setYScaleLinear();
  void setYScaleLog();

signals:
  void contextMenuRequested(QPoint pos);
  void clickedAtDataCoord(double x, double y);

protected:
  void contextMenuEvent(QContextMenuEvent *evt) override;
  void mouseReleaseEvent(QMouseEvent *evt) override;
  void setActiveCurve(std::vector<double> x, std::vector<double> y,
                      QString xunit, QString curveLabel);

private:
  QString m_activeCurveLabel;
  QStringList m_storedCurveLabels;
  QString m_xunit;
  QList<PythonObject> m_peakLabels;
};
}
}

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOT_H
