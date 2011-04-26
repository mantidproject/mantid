#ifndef XINTEGRATIONCONTROL_H_
#define XINTEGRATIONCONTROL_H_

#include "Instrument3DWidget.h"

#include <QFrame>
#include <QScrollBar>

class InstrumentWindow;
class Instrument3DWidget;

class QScrollBar;
class QPushButton;
class QLineEdit;
class QLabel;


class XIntegrationScrollBar: public QFrame
{
  Q_OBJECT
public:
  XIntegrationScrollBar(QWidget* parent);
  double getMinimum()const;
  double getMaximum()const;
  double getWidth()const;
  void set(double minimum,double maximum);
signals:
  void changed(double,double);
  void running(double,double);
protected:
   void	mouseMoveEvent (QMouseEvent * e);
   void	resizeEvent (QResizeEvent * e);
   bool eventFilter(QObject *object, QEvent *e);
   void updateMinMax();
private:
  int m_resizeMargin; ///< distance from the left (or right) end of the slider within which it can be resized
  bool m_init;
  bool m_resizingLeft;    ///< the sider is in resizing mode
  bool m_resizingRight;    ///< the sider is in resizing mode
  bool m_moving;      ///< the sider is in moving mode
  bool m_changed;
  int m_x,m_width;
  double m_minimum;
  double m_maximum;
  QPushButton* m_slider;
};

/**
  * Implements a control for setting the x integration range
  */
class XIntegrationControl: public QFrame
{
  Q_OBJECT
public:
  XIntegrationControl(InstrumentWindow* instrWindow);
  void setTotalRange(double minimum,double maximum);
  void setRange(double minimum,double maximum);
  void setWholeRange();
  double getMinimum()const;
  double getMaximum()const;
  double getWidth()const;
signals:
  void changed(double,double,bool);
private slots:
  void sliderChanged(double,double);
  void sliderRunning(double,double);
  void setMinimum();
  void setMaximum();
private:
  void updateTextBoxes();
  InstrumentWindow* m_instrWindow;
  Instrument3DWidget *mInstrumentDisplay;
  XIntegrationScrollBar* m_scrollBar;
  QLineEdit* m_minText;
  QLineEdit* m_maxText;
  double m_totalMinimum;
  double m_totalMaximum;
  double m_minimum;
  double m_maximum;
};


#endif /*XINTEGRATIONCONTROL_H_*/
