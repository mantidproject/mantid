#ifndef ONECURVEPLOT_H_
#define ONECURVEPLOT_H_

#include <qwt_plot.h>

#include <iostream>

class QwtPlotCurve;

/**
  * Implements a simple widget for plotting a single curve.
  */
class OneCurvePlot: public QwtPlot
{
  Q_OBJECT
public:
  OneCurvePlot(QWidget* parent);
  void setData(const double* x,const double* y,int dataSize);
  void setYAxisLabelRotation(double degrees);
public slots:
  void setXScale(double from, double to);
  void setYScale(double from, double to);
  void clearCurve();
  void recalcAxisDivs();
  void setYLogScale();
  void setYLinearScale();
signals:
  void showContextMenu();
  void clickedAt(double,double);
protected:
  void resizeEvent(QResizeEvent *e);
  void mousePressEvent(QMouseEvent*);
  //void mouseMoveEvent(QMouseEvent*);
  //bool event(QEvent *);
private:
  QwtPlotCurve* m_curve;
};


#endif /*ONECURVEPLOT_H_*/
