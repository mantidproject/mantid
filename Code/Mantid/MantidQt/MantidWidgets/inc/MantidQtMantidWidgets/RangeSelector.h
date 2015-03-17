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
  class PreviewPlot;

  /**
  * Allows for simpler (in a way) selection of a range on a QwtPlot in MantidQt.
  * @author Michael Whitty, RAL ISIS
  * @date 11/10/2010
  */
  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS RangeSelector : public QwtPlotPicker
  {
    Q_OBJECT
  public:
    enum SelectType { XMINMAX, XSINGLE, YMINMAX, YSINGLE };

    RangeSelector(QwtPlot* plot, SelectType type=XMINMAX, bool visible=true, bool infoOnly=false);
    RangeSelector(PreviewPlot* plot, SelectType type=XMINMAX, bool visible=true, bool infoOnly=false);
    ~RangeSelector() {};

    std::pair<double,double> getRange();
    void setRange(std::pair<double,double> range); /// Overloaded function provided for convenience

    double getMinimum() { return m_min; } ///< Returns current min value
    double getMaximum() { return m_max; } ///< Reutnrs current max value

    SelectType getType() { return m_type; }

  signals:
    void minValueChanged(double);
    void maxValueChanged(double);
    void rangeChanged(double, double);
    void selectionChanged(double, double);
    void selectionChangedLazy(double, double);

  public slots:
    void setRange(double, double);
    void setMinimum(double); ///< outside setting of value
    void setMaximum(double); ///< outside setting of value
    void reapply(); ///< re-apply the range selector lines
    void detach(); ///< Detach range selector lines from the plot
    void setColour(QColor colour);
    void setInfoOnly(bool state);
    void setVisible(bool state);

  private:
    void init();
    void setMin(double val);
    void setMax(double val);
    void setMaxMin(const double min, const double max);
    void setMinLinePos(double);
    void setMaxLinePos(double);
    void verify();
    bool inRange(double);
    bool changingMin(double, double);
    bool changingMax(double, double);
    bool eventFilter(QObject*, QEvent*);

    // MEMBER ATTRIBUTES
    SelectType m_type; ///< type of selection widget is for

    double m_min;
    double m_max;
    double m_lower; ///< lowest allowed value for range
    double m_higher; ///< highest allowed value for range

    QwtPlotCanvas* m_canvas;
    QwtPlot* m_plot;

    QwtPlotMarker* m_mrkMin;
    QwtPlotMarker* m_mrkMax;

    bool m_minChanging;
    bool m_maxChanging;

    bool m_infoOnly;
    bool m_visible;

    /** Strictly UI options and settings below this point **/

    QPen* m_pen; ///< pen object used to define line style, colour, etc
    QCursor m_movCursor; ///< the cursor object to display when an item is being moved

  };

} // MantidWidgets
} // MantidQt

#endif
