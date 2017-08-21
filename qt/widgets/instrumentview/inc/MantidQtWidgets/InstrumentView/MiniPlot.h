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

#include <QActionGroup>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class MplFigureCanvas;
}
}

namespace MantidWidgets {

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
class MiniPlot : public QWidget {
  Q_OBJECT

public:
  MiniPlot(QWidget *parent = nullptr);

  // Query
  bool hasStoredCurves() const;
  bool hasActiveCurve() const;

  // Modify
  /// Redraw the canvas
  void update();
  void setActiveCurve(std::vector<double> x, std::vector<double> y,
                      QString xunit, QString curveLabel);
  void setActiveCurve(MiniPlotCurveData data);
  void removeActiveCurve();

public slots:
  void storeCurve();
  void setYScaleLinear();
  void setYScaleLog();

protected:
  bool eventFilter(QObject *watched, QEvent *evt) override;
  void contextMenuEvent(QContextMenuEvent *evt) override;

private:
  void initActions();

  Widgets::MplCpp::MplFigureCanvas *m_canvas;
  QString m_activeCurveLabel;
  QStringList m_storedCurveLabels;

  // actions
  QActionGroup *m_yScaleActions;
  QAction *m_linearY;
  QAction *m_logY;
};
}
}

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOT_H
