#ifndef COLORBARWIDGET_H
#define COLORBARWIDGET_H

#include <QtGui/QWidget>
#include "ui_ColorBarWidget.h"
#include <qwt_color_map.h>
#include <qwt_scale_widget.h>

class ColorBarWidget : public QWidget
{
  Q_OBJECT

public:
  ColorBarWidget(QWidget *parent = 0);
  ~ColorBarWidget();

  void setColorMap(QwtColorMap * colorMap);
  void setDataRange(double min, double max);
  void setViewRange(double min, double max);
  void setLog(bool log);

  double getMinimum() const;
  double getMaximum() const;
  bool getLog() const;

public slots:
  void changedLogState(int);
  void changedMinimum(double);
  void changedMaximum(double);

signals:
  void changedColorRange(double min, double max, bool log);

private:
  void setSpinBoxesSteps();
  void update();

  /// Auto-gen UI classes
  Ui::ColorBarWidgetClass ui;

  /// The color bar widget from QWT
  QwtScaleWidget * m_colorBar;

  /// Color map being displayed
  QwtColorMap * m_colorMap;

  /// Logarithmic scale?
  bool m_log;

  /// Min value in the data shown
  double m_rangeMin;

  /// Max value in the data shown
  double m_rangeMax;

  /// Min value being displayed
  double m_min;

  /// Min value being displayed
  double m_max;
};

#endif // COLORBARWIDGET_H
