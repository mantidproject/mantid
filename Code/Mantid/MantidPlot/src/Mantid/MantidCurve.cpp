#include "MantidCurve.h"

#include <qpainter.h>
#include <qwt_symbol.h>

#include "MantidAPI/AnalysisDataService.h"

#include "../Graph.h"
#include "../ApplicationWindow.h"
#include "../MultiLayer.h"

/**
Constructor
@param wsName : Name of the workspace
*/
MantidCurve::MantidCurve(const QString& wsName) : PlotCurve(wsName), WorkspaceObserver()
{
}

/**
Constructor default
*/
MantidCurve::MantidCurve() : PlotCurve(), WorkspaceObserver()
{
}

/**
Helper method to apply a chosen style.
@param style : The chosen graph type style
@parm ml : pointer to multilayer object
@param linewidth: ref to linewidth, which may be internally adjusted
*/
void MantidCurve::applyStyleChoice(Graph::CurveType style, MultiLayer* ml, int& lineWidth)
{

  if ( style == Graph::Unspecified )
    style = static_cast<Graph::CurveType>(ml->applicationWindow()->defaultCurveStyle);

  QwtPlotCurve::CurveStyle qwtStyle;
  const int symbolSize = ml->applicationWindow()->defaultSymbolSize;
  const QwtSymbol symbol(QwtSymbol::Ellipse, QBrush(Qt::black),QPen(),QSize(symbolSize,symbolSize));
  switch(style)
  {
  case Graph::Line :
    qwtStyle = QwtPlotCurve::Lines;
    break;
  case Graph::Scatter:
    qwtStyle = QwtPlotCurve::NoCurve;
    this->setSymbol(symbol);
    break;
  case Graph::LineSymbols :
    qwtStyle = QwtPlotCurve::Lines;
    this->setSymbol(symbol);
    break;
  case 15:
    qwtStyle = QwtPlotCurve::Steps;
    break;  // should be Graph::HorizontalSteps but it doesn't work
  default:
    qwtStyle = QwtPlotCurve::Lines;
    break;
  }
  setStyle(qwtStyle);
  lineWidth = static_cast<int>(floor(ml->applicationWindow()->defaultCurveLineWidth));
}

/**
Rebuild the bounding rectangle. Uses the mantidData (QwtMantidWorkspaceData) object to do so.
*/
QwtDoubleRect MantidCurve::boundingRect() const
{
  if (m_boundingRect.isNull())
  {
    const QwtData* data = mantidData();
    if (data->size() == 0) return QwtDoubleRect(0,0,1,1);
    double y_min = std::numeric_limits<double>::infinity();
    double y_max = -y_min;
    for(size_t i=0;i<data->size();++i)
    {
      double y = data->y(i);
      if (y == std::numeric_limits<double>::infinity() || y != y) continue;
      if (y < y_min && (!mantidData()->logScale() || y > 0.)) y_min = y;
      if (y > y_max) y_max = y;
    }
    double x_min = data->x(0);
    double x_max = data->x(data->size()-1);
    m_boundingRect = QwtDoubleRect(x_min,y_min,x_max-x_min,y_max-y_min);
  }
  return m_boundingRect;
}

/**
Slot for axis scale changed. Invalidate and rebuild the bounding rectangle.
@param axis: axis index.
@param toLog: true if switching to a log value
*/
void MantidCurve::axisScaleChanged(int axis, bool toLog)
{
  if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight)
  {
    mantidData()->setLogScale(toLog);
    // force boundingRect calculation at this moment
    invalidateBoundingRect();
    boundingRect();
    mantidData()->saveLowestPositiveValue(m_boundingRect.y());
  }
}

/// Destructor
MantidCurve::~MantidCurve()
{
}

