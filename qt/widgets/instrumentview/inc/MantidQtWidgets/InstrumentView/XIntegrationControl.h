// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QFrame>
#include <QScrollBar>

class Instrument3DWidget;

class QScrollBar;
class QPushButton;
class QLineEdit;
class QLabel;
class QSpinBox;

namespace MantidQt {
namespace MantidWidgets {

class InstrumentWidget;

class XIntegrationScrollBar : public QFrame {
  Q_OBJECT
public:
  explicit XIntegrationScrollBar(QWidget *parent);
  double getMinimum() const;
  double getMaximum() const;
  double getWidth() const;
  void set(double minimum, double maximum);
  void setStepsTotal(int steps);
  void setDiscrete(bool isDiscrete) { m_isDiscrete = isDiscrete; }
signals:
  void changed(double /*_t1*/, double /*_t2*/);
  void running(double /*_t1*/, double /*_t2*/);

protected:
  void mouseMoveEvent(QMouseEvent *e) override;
  void resizeEvent(QResizeEvent *e) override;
  bool eventFilter(QObject *object, QEvent *e) override;
  bool manageEventContinuous(QEvent *e);
  bool manageEventDiscrete(QEvent *e);
  void updateMinMax();
  void setDiscreteValues();

private:
  int m_resizeMargin; ///< distance from the left (or right) end of the slider within which it can be resized
  bool m_init;
  bool m_resizingLeft;  ///< the slider is in resizing mode
  bool m_resizingRight; ///< the slider is in resizing mode
  bool m_moving;        ///< the slider is in moving mode
  bool m_changed;       ///< the slider has been changed since the mouse was clicked
  int m_x, m_width;  ///< variables to keep track of the original x pos and size of the slider, used when it is moving
  double m_minimum;  ///< the min of the current integration range, between 0 and 1
  double m_maximum;  ///< the max of the current integration range, between 0 and 1
  int m_stepsTotal;  ///< in a discrete slider case, the total number of steps possible in total
  bool m_isDiscrete; ///< whether the slider should have discrete, integer values
  int m_currentStepMin; ///< in a discrete slider case, current index of the min step integrated
  int m_currentStepMax; ///< in a discrete slider case, current index of the max step integrated
  QPushButton *m_slider;
};

/**
 * Implements a control for setting the x integration range
 */
class XIntegrationControl : public QFrame {
  Q_OBJECT
public:
  explicit XIntegrationControl(InstrumentWidget *instrWindow);
  void setTotalRange(double minimum, double maximum);
  void setUnits(const QString &units);
  void setRange(double minimum, double maximum);
  void setDiscrete(bool isDiscrete);
  void setStepsTotal(int steps) { m_scrollBar->setStepsTotal(steps); }
  double getMinimum() const;
  double getMaximum() const;
  double getWidth() const;
signals:
  void changed(double /*_t1*/, double /*_t2*/);
public slots:
  void setWholeRange();
private slots:
  void sliderChanged(double /*minimum*/, double /*maximum*/);
  void sliderRunning(double /*minimum*/, double /*maximum*/);
  void setMinimum();
  void setMaximum();

private:
  void updateTextBoxes();
  void discretize();
  InstrumentWidget *m_instrWindow;
  XIntegrationScrollBar *m_scrollBar;
  QLineEdit *m_minText;
  QLineEdit *m_maxText;
  QSpinBox *m_minSpin;
  QSpinBox *m_maxSpin;
  QLabel *m_units;
  QPushButton *m_setWholeRange;
  double m_totalMinimum;
  double m_totalMaximum;
  double m_minimum;
  double m_maximum;
  bool m_isDiscrete;
};
} // namespace MantidWidgets
} // namespace MantidQt
