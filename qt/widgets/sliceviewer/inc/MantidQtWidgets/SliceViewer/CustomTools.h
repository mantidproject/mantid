// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CUSTOMTOOLS_H_
#define CUSTOMTOOLS_H_
#include <QMouseEvent>
#include <qwt_picker_machine.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
/*
 * CustomTools.h
 *
 * Some customized versions of QwtTools for the slice viewer
 *
 *  Created on: Oct 12, 2011
 *      Author: Janik zikovsky
 */

namespace MantidQt {
namespace SliceViewer {

//========================================================================
class PickerMachine : public QwtPickerMachine {
public:
  QwtPickerMachine::CommandList transition(const QwtEventPattern & /*unused*/,
                                           const QEvent *e) override {
    QwtPickerMachine::CommandList cmdList;
    if (e->type() == QEvent::MouseMove)
      cmdList += Move;

    return cmdList;
  }
};

//========================================================================
/** Customized QwtPlotMagnifier for zooming in on the view */
class CustomMagnifier : public QwtPlotMagnifier {
  Q_OBJECT
public:
  CustomMagnifier(QwtPlotCanvas *canvas) : QwtPlotMagnifier(canvas) {}
signals:
  /// Signal to emitted upon scaling.
  void rescaled(double factor) const;

protected:
  /** Method to flip the way the wheel operates */
  void rescale(double factor) override;
};

/** Picker for looking at the data under the mouse */
class CustomPicker : public QwtPlotPicker {
  Q_OBJECT

public:
  CustomPicker(int xAxis, int yAxis, QwtPlotCanvas *canvas);
  void widgetMouseMoveEvent(QMouseEvent *e) override;
  void widgetLeaveEvent(QEvent * /*unused*/) override;

  QwtPickerMachine *stateMachine(int /*unused*/) const override {
    return new PickerMachine;
  }

signals:
  void mouseMoved(double /*x*/, double /*y*/) const;

protected:
  // Unhide base class method (avoids Intel compiler warning)
  using QwtPlotPicker::trackerText;
  QwtText trackerText(const QwtDoublePoint &pos) const override;
};

//========================================================================
/** Custom zoomer for zooming onto the slice */
class CustomZoomer : public QwtPlotZoomer {
public:
  CustomZoomer(QwtPlotCanvas *canvas) : QwtPlotZoomer(canvas) {
    setTrackerMode(QwtPicker::AlwaysOn);
  }

protected:
  // Unhide base class method (avoids Intel compiler warning)
  using QwtPlotZoomer::trackerText;
  QwtText trackerText(const QwtDoublePoint &p) const override {
    QwtText t(QwtPlotPicker::trackerText(p));
    QColor c(Qt::white);
    c.setAlpha(120);
    t.setBackgroundBrush(QBrush(c));
    return t;
  }
};

} // namespace SliceViewer
} // namespace MantidQt

#endif /* CUSTOMTOOLS_H_ */
