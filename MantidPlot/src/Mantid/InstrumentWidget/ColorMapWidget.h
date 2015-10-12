#ifndef COLORMAPWIDGET_H_
#define COLORMAPWIDGET_H_

#include <QFrame>
#include <src/lib/include/DoubleSpinBox.h>

class MantidColorMap;

class QwtScaleWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class DoubleSpinBox;


/**
  * Displays a color map with numeric axis and editable bounds
  */
class ColorMapWidget: public QFrame
{
  Q_OBJECT
  enum DragType {Bottom,Top};
public:
  ColorMapWidget(int type,QWidget* parent,const double& minPositiveValue = 0.0001);
  void setupColorBarScaling(const MantidColorMap&);
  void setMinValue(double);
  void setMaxValue(double);
  void setMinPositiveValue(double);
  int getScaleType()const;
  void setScaleType(int);
  void setNthPower(double);
signals:
  void scaleTypeChanged(int);
  void minValueChanged(double);
  void maxValueChanged(double);
  void nthPowerChanged(double);
protected:
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void updateScale();
  void setMinValueText(double);
  void setMaxValueText(double);
private slots:
  void scaleOptionsChanged(int);
  void nPowerChanged(double);
  void minValueChanged();
  void maxValueChanged();
private:
  QwtScaleWidget *m_scaleWidget;
  QLineEdit *m_minValueBox, *m_maxValueBox;
  QComboBox *m_scaleOptions;
  QLabel *m_lblN;
  DoubleSpinBox *m_dspnN;
  double m_minPositiveValue;
  bool m_dragging;
  int m_y;
  DragType m_dtype;
  double m_nth_power;
};


#endif /*COLORMAPWIDGET_H_*/
