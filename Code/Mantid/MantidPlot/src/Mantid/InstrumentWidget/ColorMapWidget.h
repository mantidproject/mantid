#ifndef COLORMAPWIDGET_H_
#define COLORMAPWIDGET_H_

#include "Instrument3DWidget.h"

#include <QFrame>

class MantidColorMap;

class QwtScaleWidget;
class QLineEdit;
class QComboBox;


/**
  * Displays a color map with numeric axis and editable bounds
  */
class ColorMapWidget: public QFrame
{
  Q_OBJECT
public:
  ColorMapWidget(int type,QWidget* parent);
  void setupColorBarScaling(const MantidColorMap&);
  void setMinValue(double);
  void setMaxValue(double);
  int getScaleType()const;
  void setScaleType(int);
signals:
  void scaleTypeChanged(int);
  void minValueChanged(double);
  void maxValueChanged(double);
private slots:
  void scaleOptionsChanged(int);
  void minValueChanged();
  void maxValueChanged();
private:
  QwtScaleWidget *m_scaleWidget;
  QLineEdit *m_minValueBox, *m_maxValueBox;
  QComboBox *m_scaleOptions;
};


#endif /*COLORMAPWIDGET_H_*/
