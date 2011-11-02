#ifndef COLORBARWIDGET_H
#define COLORBARWIDGET_H

#include <QtGui/QWidget>
#include "ui_ColorBarWidget.h"
#include <qwt_color_map.h>
#include <qwt_scale_widget.h>
#include "MantidQtAPI/MantidColorMap.h"

class ColorBarWidget : public QWidget
{
  Q_OBJECT

public:
  ColorBarWidget(QWidget *parent = 0);
  ~ColorBarWidget();

  void update();

  void setDataRange(double min, double max);
  void setDataRange(QwtDoubleInterval range);
  void setViewRange(double min, double max);
  void setViewRange(QwtDoubleInterval range);
  void setLog(bool log);

  double getMinimum() const;
  double getMaximum() const;
  bool getLog() const;
  QwtDoubleInterval getViewRange() const;
  MantidColorMap & getColorMap();

public slots:
  void changedLogState(int);
  void changedMinimum();
  void changedMaximum();

signals:
  /// Signal sent when the range or log mode of the color scale changes.
  void changedColorRange(double min, double max, bool log);
  /// When the user double-clicks the color bar (e.g. load a new color map)
  void colorBarDoubleClicked();

private:
  void setSpinBoxesSteps();
  void mouseDoubleClickEvent(QMouseEvent * event);

  /// Auto-gen UI classes
  Ui::ColorBarWidgetClass ui;

  /// The color bar widget from QWT
  QwtScaleWidget * m_colorBar;

  /// Color map being displayed
  MantidColorMap m_colorMap;

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
