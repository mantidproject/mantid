//---------------------------
// Includes
//--------------------------

#include "GridDetails.h"
#include "ApplicationWindow.h"
#include <qwt_scale_widget.h>
//#include <qwt_plot.h>
#include "qwt_compat.h"
#include "Plot.h"
#include "plot2D/ScaleEngine.h"

#include <QWidget>

GridDetails::GridDetails(ApplicationWindow* app, Graph* graph, QWidget *parent) : QWidget(parent)
{
  d_app = app;
  d_graph = graph;
  m_initialised = false;

  initWidgets();
}

GridDetails::~GridDetails()
{

}

void GridDetails::initWidgets()
{
  if (m_initialised)
  {
    return;
  }
  else
  {
    Plot *p = d_graph->plotWidget();
    


    m_modified = false;
    m_initialised = true;
  }
}

void GridDetails::setModified()
{
  m_modified = true;
}

bool GridDetails::valid()
{
  return true;
}

void GridDetails::apply()
{
  if (!(d_app && d_graph && valid()) || !m_modified)
    return;

  m_modified = false;
}