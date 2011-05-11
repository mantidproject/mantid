#ifndef ONECURVEPLOT_H_
#define ONECURVEPLOT_H_

#include <qwt_plot.h>
#include <QList>

class QwtPlotCurve;
class QwtPlotZoomer;

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
  void contextMenuEvent (QContextMenuEvent *e);
  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
private:
  QwtPlotCurve* m_curve;
  QwtPlotZoomer* m_zoomer; ///< does zooming
  int m_x0; ///< save x coord of last left mouse click
  int m_y0; ///< save y coord of last left mouse click
};


#endif /*ONECURVEPLOT_H_*/
