#ifndef DATAPOINTMARKER_H
#define DATAPOINTMARKER_H

#include "LabelTool.h"
#include "PlotEnrichement.h"

#include <qwt_painter.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_canvas.h>
#include <qwt_symbol.h>
#include <qwt_text.h>
#include <qwt_text_label.h>

#include "Plot.h"
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>

class QwtText;
class QwtSymbol;

/// Draws symbols on a QwtPlot.
class DataPointMarker: public QObject, public PlotEnrichement
{
	//Q_OBJECT
  public:
    DataPointMarker(Plot *);

	//QwtPlot *plot(){return d_plot;};
  
  // Sets the position of the marker in plot coordinates.
	void setMarkerPlotPos(double x, double y);
  
  //Sets the x and y values of the data point.
  void setXValue(double x);
  void setYValue(double y);

  // Returns the x and y plot coordinate values of the data point.
  double xPlotPosOfDataPoint();
  double yPlotPosOfDataPoint();

  // Returns the x and y paint coordinate values of the data point.
  int xPaintPosOfDataPoint();
  int yPaintPosOfDataPoint();

  /*
  / Draws the symbol on the graph.
  virtual void draw(QPainter *p, 
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &) const;
    */

	//! Set position (m_positionX and m_positionY, right and bottom values giving everything in plot coordinates.
	//void setBoundingRect(double left, double top, double right, double bottom);
  //! Set value (position) and #d_size, giving everything in paint coordinates.
	//void setRect(int x, int y, int w, int h);


  //! Return bounding rectangle in plot coordinates.
  //virtual QwtDoubleRect boundingRect() const;
  //! Return bounding rectangle in paint coordinates.
  // QRect rect() const;
 
  //! Recalculates the bounding rectangle in values coordinates using the pixel coordinats when the scales change.
	//void updateBoundingRect();
  
	  
  /*
  //! Set QwtPlotMarker::value() in paint coordinates.
	void setOrigin(const QPoint &p);
  
   //!Sets a corresponding label to go with the marker.
   
  void setLabel(const QwtText&);
  QwtText label() const;
  
  */

  double right(){return d_x_right;};
	double bottom(){return d_y_bottom;};

private:
  
  //! Parent plot
	//Plot *d_plot;

	// Pixel coordinates of the marker.
	double m_positionX;
  double m_positionY;

  // cppcheck-suppress duplInheritedMember
	double d_x_right;   //!< The right side position in scale coordinates.
  // cppcheck-suppress duplInheritedMember
  double d_y_bottom;  //!< The bottom side position in scale coordinates.
  // cppcheck-suppress duplInheritedMember
	QPoint d_pos;       //!< The position in paint coordinates.
	// cppcheck-suppress duplInheritedMember
	QSize d_size;       //!< The size (in paint coordinates) to which the bounding rect will be scaled in draw().
};
#endif
