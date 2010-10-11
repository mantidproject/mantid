#ifndef MANTIDQT_MANTIDWIDGET_POSHPLOTTING_H
#define MANTIDQT_MANTIDWIDGET_POSHPLOTTING_H

#include "WidgetDllOption.h"

#include <qwt_plot_picker.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>

#include <QPen>

namespace MantidQt
{
namespace MantidWidgets
{
  /**
  * Allows for simpler (in a way) selection of a range on a QwtPlot in MantidQt.
  * @author Michael Whitty, RAL ISIS
  * @date 11/10/2010
  */
  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS RangeSelector : public QwtPlotPicker
  {
    Q_OBJECT
  public:
    RangeSelector(QwtPlot*);
    ~RangeSelector() {};

    bool eventFilter(QObject*, QEvent*);
    
    void setRange(double, double);
    bool changingXMin(double, double);
    bool changingXMax(double, double);

  signals:
    void xMinValueChanged(double);
    void xMaxValueChanged(double);

  public slots:
    void xMinChanged(double);
    void xMaxChanged(double);
    void setMinimum(double);
    void setMaximum(double);
    void reapply(); ///< re-apply the range selector lines

  private:
    void setXMin(double val);
    void setXMax(double val);
    void verify();
    bool inRange(double);

    double m_xMin;
    double m_xMax;

    double m_lower; ///< lowest allowed value for range
    double m_higher; ///< highest allowed value for range


    QwtPlotCanvas* m_canvas;
    QwtPlot* m_plot;

    QwtPlotMarker* m_mrkMin;
    QwtPlotMarker* m_mrkMax;

    bool m_xMinChanging;
    bool m_xMaxChanging;

    QPen* m_pen; ///< pen object used to define line style, colour, etc

  };

} // MantidWidgets
} // MantidQt

#endif