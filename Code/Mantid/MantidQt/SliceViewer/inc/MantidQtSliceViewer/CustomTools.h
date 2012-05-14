#ifndef CUSTOMTOOLS_H_
#define CUSTOMTOOLS_H_
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_canvas.h>
#include <qwt_picker_machine.h>
#include <QMouseEvent>
/*
 * CustomTools.h
 *
 * Some customized versions of QwtTools for the slice viewer
 *
 *  Created on: Oct 12, 2011
 *      Author: Janik zikovsky
 */

namespace MantidQt
{
namespace SliceViewer
{


//========================================================================
class PickerMachine : public QwtPickerMachine
{
public:
  virtual QwtPickerMachine::CommandList transition(
      const QwtEventPattern &, const QEvent *e)
  {
    QwtPickerMachine::CommandList cmdList;
    if ( e->type() == QEvent::MouseMove )
    cmdList += Move;

    return cmdList;
  }
};

/** Picker for looking at the data under the mouse */
class CustomPicker : public QwtPlotPicker
{
  Q_OBJECT

public:
  CustomPicker(int xAxis, int yAxis, QwtPlotCanvas* canvas);
  void widgetMouseMoveEvent(QMouseEvent *e);
  void widgetLeaveEvent(QEvent *);

  virtual QwtPickerMachine *stateMachine(int) const
  {
    return new PickerMachine;
  }

signals:
  void mouseMoved(double /*x*/, double /*y*/) const;

protected:
  // Unhide base class method (avoids Intel compiler warning)
  using QwtPlotPicker::trackerText;
  QwtText trackerText (const QwtDoublePoint & pos) const;
};



//========================================================================
/** Custom zoomer for zooming onto the slice */
class CustomZoomer: public QwtPlotZoomer
{
public:
  CustomZoomer(QwtPlotCanvas* canvas): QwtPlotZoomer(canvas)
  {
    setTrackerMode(QwtPicker::AlwaysOn);
  }

protected:
  // Unhide base class method (avoids Intel compiler warning)
  using QwtPlotZoomer::trackerText;
  virtual QwtText trackerText( const QwtDoublePoint& p ) const
  {
    QwtText t( QwtPlotPicker::trackerText( p ));
    QColor c(Qt::white);
    c.setAlpha(120);
    t.setBackgroundBrush( QBrush(c) );
    return t;
  }
};

//========================================================================
/** Customized QwtPlotMagnifier for zooming in on the view */
class CustomMagnifier : public QwtPlotMagnifier
{
public:
  CustomMagnifier(QwtPlotCanvas* canvas): QwtPlotMagnifier(canvas)
  {
  }

protected:
  /** Method to flip the way the wheel operates */
  virtual void rescale(double factor)
  {
    if ( factor != 0.0 )
      QwtPlotMagnifier::rescale(1 / factor);
  }
};


} // namespace SliceViewer
} // namespace Mantid

#endif /* CUSTOMTOOLS_H_ */
