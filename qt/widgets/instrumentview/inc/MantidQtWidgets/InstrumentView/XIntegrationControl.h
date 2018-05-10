#ifndef XINTEGRATIONCONTROL_H_
#define XINTEGRATIONCONTROL_H_

#include <QFrame>
#include <QScrollBar>

class Instrument3DWidget;

class QScrollBar;
class QPushButton;
class QLineEdit;
class QLabel;

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
signals:
  void changed(double, double);
  void running(double, double);

protected:
  void mouseMoveEvent(QMouseEvent *e) override;
  void resizeEvent(QResizeEvent *e) override;
  bool eventFilter(QObject *object, QEvent *e) override;
  void updateMinMax();

private:
  int m_resizeMargin; ///< distance from the left (or right) end of the slider
  /// within which it can be resized
  bool m_init;
  bool m_resizingLeft;  ///< the sider is in resizing mode
  bool m_resizingRight; ///< the sider is in resizing mode
  bool m_moving;        ///< the sider is in moving mode
  bool m_changed;
  int m_x, m_width;
  double m_minimum;
  double m_maximum;
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
  double getMinimum() const;
  double getMaximum() const;
  double getWidth() const;
signals:
  void changed(double, double);
public slots:
  void setWholeRange();
private slots:
  void sliderChanged(double, double);
  void sliderRunning(double, double);
  void setMinimum();
  void setMaximum();

private:
  void updateTextBoxes();
  InstrumentWidget *m_instrWindow;
  XIntegrationScrollBar *m_scrollBar;
  QLineEdit *m_minText;
  QLineEdit *m_maxText;
  QLabel *m_units;
  QPushButton *m_setWholeRange;
  double m_totalMinimum;
  double m_totalMaximum;
  double m_minimum;
  double m_maximum;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*XINTEGRATIONCONTROL_H_*/
