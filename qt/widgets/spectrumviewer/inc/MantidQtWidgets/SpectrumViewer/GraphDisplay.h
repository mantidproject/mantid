// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GRAPH_DISPLAY_H
#define GRAPH_DISPLAY_H

#include <QTableWidget>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"

/**
    @class GraphDisplay

    This class handles the display of vertical and horizontal cuts
    through the data in an SpectrumView display.

    @author Dennis Mikkelson
    @date   2012-04-03
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER GraphDisplay {
public:
  /// Construct a GraphDisplay to display in the specifed plot and table
  GraphDisplay(QwtPlot *graphPlot, QTableWidget *graphTable, bool isVertical);

  ~GraphDisplay();

  /// Set the source of information for the table of position information
  void setDataSource(SpectrumDataSource_sptr dataSource);

  /// Set the actual data that will be displayed on the graph
  void setData(const QVector<double> &xData, const QVector<double> &yData,
               double cutValue, bool isFront = true);

  /// Clear the graph(s) off the display
  void clear();

  /// Set up axes using the specified scale factor and replot the graph
  void setRangeScale(double rangeScale);

  /// Set flag indicating whether or not to use a log scale on the x-axis
  void setLogX(bool isLogX);

  /// Record the point that the user is currently pointing at with the mouse
  void setPointedAtPoint(QPoint point);

  /// Get the pointed currently being pointed at
  QPoint getPointedAtPoint() const { return m_mousePoint; }

private:
  /// Show information about the point (x, y) on the graph, in the info table
  void showInfoList(double x, double y);
  /// Remove all curves.
  void clearCurves();

  QwtPlot *m_graphPlot;
  QList<QwtPlotCurve *> m_curves;
  QTableWidget *m_graphTable;
  SpectrumDataSource_sptr m_dataSource;

  bool m_isVertical;
  bool m_isLogX;
  double m_imageX;
  double m_imageY;
  double m_rangeScale; // Fraction of data range to be graphed
  double m_minX;
  double m_maxX;
  double m_minY;
  double m_maxY;
  QPoint m_mousePoint;

  static std::vector<QColor> g_curveColors;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // GRAPH_DISPLAY_H
