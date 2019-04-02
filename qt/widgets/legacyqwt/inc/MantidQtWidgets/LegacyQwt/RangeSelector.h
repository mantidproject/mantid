// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGET_POSHPLOTTING_H
#define MANTIDQT_MANTIDWIDGET_POSHPLOTTING_H

#include "DllOption.h"

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>

#include <QPen>

namespace MantidQt {
namespace MantidWidgets {
class PreviewPlot;

/**
 * Allows for simpler (in a way) selection of a range on a QwtPlot in MantidQt.
 * @author Michael Whitty, RAL ISIS
 * @date 11/10/2010
 */
class EXPORT_OPT_MANTIDQT_LEGACYQWT RangeSelector : public QwtPlotPicker {
  Q_OBJECT
public:
  enum SelectType { XMINMAX, XSINGLE, YMINMAX, YSINGLE };

  RangeSelector(QwtPlot *plot, SelectType type = XMINMAX, bool visible = true,
                bool infoOnly = false);
  RangeSelector(PreviewPlot *plot, SelectType type = XMINMAX,
                bool visible = true, bool infoOnly = false);
  ~RangeSelector() override{};

  std::pair<double, double> getRange();
  void setRange(std::pair<double, double>
                    range); /// Overloaded function provided for convenience

  double getMinimum() { return m_min; } ///< Returns current min value
  double getMaximum() { return m_max; } ///< Reutnrs current max value

  SelectType getType() { return m_type; }
  bool isVisible() { return m_visible; }

signals:
  void minValueChanged(double /*_t1*/);
  void maxValueChanged(double /*_t1*/);
  void rangeChanged(double /*_t1*/, double /*_t2*/);
  void selectionChanged(double /*_t1*/, double /*_t2*/);
  void selectionChangedLazy(double /*_t1*/, double /*_t2*/);

public slots:
  void setRange(double /*min*/, double /*max*/);
  void setMinimum(double /*val*/); ///< outside setting of value
  void setMaximum(double /*val*/); ///< outside setting of value
  void reapply();                  ///< re-apply the range selector lines
  void detach(); ///< Detach range selector lines from the plot
  void setColour(QColor colour);
  void setInfoOnly(bool state);
  void setVisible(bool state);

private:
  void init();
  void setMin(double val);
  void setMax(double val);
  void setMaxMin(const double min, const double max);
  void setMinLinePos(double /*val*/);
  void setMaxLinePos(double /*val*/);
  void verify();
  bool inRange(double /*x*/);
  bool changingMin(double /*x*/, double /*xPlusdx*/);
  bool changingMax(double /*x*/, double /*xPlusdx*/);
  bool eventFilter(QObject * /*unused*/, QEvent * /*unused*/) override;

  // MEMBER ATTRIBUTES
  SelectType m_type; ///< type of selection widget is for

  /// current position of the line marking the minimum
  double m_min;
  /// current position of the line marking the maximum
  double m_max;
  /// lowest allowed position of the line marking the minimum
  double m_lower;
  /// highest allowed position of the line marking the maximum
  double m_higher;
  /// the line object in the plot marking the position of the minimum
  QwtPlotMarker *m_mrkMin;
  /// the line object in the plot marking the position of the maximum
  QwtPlotMarker *m_mrkMax;

  /// widget receiving the  marker lines to be plotted
  QwtPlot *m_plot;
  /// the actual area of m_plot where the marker lines are plotted
  QwtPlotCanvas *m_canvas;

  /// signals the position of the line marking the minimum is to be changed
  bool m_minChanging;
  /// signals the position of the line marking the maximum is to be changed
  bool m_maxChanging;

  bool m_infoOnly;
  /// whether the lines should be visible
  bool m_visible;

  /** Strictly UI options and settings below this point **/

  QPen *m_pen; ///< pen object used to define line style, colour, etc
  QCursor
      m_movCursor; ///< the cursor object to display when an item is being moved
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif
