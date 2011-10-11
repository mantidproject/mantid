#include "DimensionSliceWidget.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "QwtRasterDataMD.h"
#include "SliceViewer.h"
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <qwt_color_map.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <vector>
#include <qwt_scale_engine.h>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;


SliceViewer::SliceViewer(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);

	// Create the plot
  m_spectLayout = new QVBoxLayout(ui.frmPlot);
	m_plot = new QwtPlot();
  m_plot->autoRefresh();
  m_spectLayout->addWidget(m_plot, 1, 0);

	// Add a spectrograph
	m_spect = new QwtPlotSpectrogram();
	m_spect->attach(m_plot);

	m_data = new QwtRasterDataMD();
	m_spect->setData(*m_data);
	m_spect->setColorMap(QwtLinearColorMap(Qt::black, Qt::white));
	m_spect->itemChanged();
  m_plot->autoRefresh();
}

SliceViewer::~SliceViewer()
{

}


///** Get a slice from the workspace with the current settings */
//SliceViewer::getSlice(double * data, size_t & length)
//{
//}

//------------------------------------------------------------------------------------
/** Reset the axis and scale it
 *
 * @param axis :: int for X or Y
 * @param dim :: dimension to show
 */
void SliceViewer::resetAxis(int axis, Mantid::Geometry::IMDDimension_const_sptr dim)
{
  m_plot->setAxisScale( axis, dim->getMinimum(), dim->getMaximum(), (dim->getMaximum()-dim->getMinimum())/5);
//  m_plot->setAxisAutoScale(axis);
  m_plot->setAxisTitle( axis, QString::fromStdString(dim->getName() + " (" + dim->getUnits() + ")") );
//  QwtLinearScaleEngine * engine = new QwtLinearScaleEngine();
//  m_plot->setAxisScaleEngine( axis, engine);
}

//------------------------------------------------------------------------------------
/** Update the 2D plot using all the current controls settings */
void SliceViewer::updateDisplay()
{
  m_data->timesRequested = 0;

  std::cout << "SliceViewer::updateDisplay()\n";
  size_t dimX = 0;
  size_t dimY = 1;
  std::vector<coord_t> slicePoint;
  for (size_t d=0; d<m_ws->getNumDims(); d++)
  {
    DimensionSliceWidget * widget = m_dimWidgets[d];
    if (widget->getShownDim() == 0)
      dimX = d;
    if (widget->getShownDim() == 1)
      dimY = d;
    slicePoint.push_back(widget->getSlicePoint());
  }
  m_data->setSliceParams(dimX, dimY, slicePoint);

  m_X = m_ws->getDimension(dimX);
  m_Y = m_ws->getDimension(dimY);

  // Reset the 2 axes to full scale
  this->resetAxis(m_spect->xAxis(), m_X );
  this->resetAxis(m_spect->yAxis(), m_Y );

  // Notify the graph that the underlying data changed
  m_spect->setData(*m_data);
  m_spect->itemChanged();
  m_plot->replot();
  std::cout << m_plot->sizeHint().width() << " width\n";

}



//------------------------------------------------------------------------------------
/** The user changed the shown dimension somewhere.
 *
 * @param index :: index of the dimension
 * @param dim :: shown dimension, 0=X, 1=Y, -1 sliced
 * @param dim :: previous shown dimension, 0=X, 1=Y, -1 sliced
 */
void SliceViewer::changedShownDim(int index, int dim, int oldDim)
{
  if (dim >= 0)
  {
    // Swap from X to Y
    if (oldDim >= 0 && oldDim != dim)
    {
      for (size_t d=0; d<m_ws->getNumDims(); d++)
      {
        // A different dimension had the same shown dimension
        if ((size_t(index) != d) &&
            (m_dimWidgets[d]->getShownDim() == dim))
        {
          // So flip it. If the new one is X, the old one becomes Y
          m_dimWidgets[d]->setShownDim( (dim==0) ? 1 : 0 );
          break;
        }
      }
    }
    // Make sure no other dimension is showing the same one
    for (size_t d=0; d<m_ws->getNumDims(); d++)
    {
      // A different dimension had the same shown dimension
      if ((size_t(index) != d) &&
          (m_dimWidgets[d]->getShownDim() == dim))
      {
        m_dimWidgets[d]->setShownDim(-1);
      }
    }
  }
  this->updateDisplay();
}

/** Slot to redraw when the slice poitn changes */
void SliceViewer::updateDisplaySlot(int index, double value)
{
  UNUSED_ARG(index)
  UNUSED_ARG(value)
  this->updateDisplay();
}


//------------------------------------------------------------------------------------
/** Add (as needed) and update DimensionSliceWidget's. */
void SliceViewer::updateDimensionSliceWidgets()
{
  // Create all necessary widgets
  if (m_dimWidgets.size() < m_ws->getNumDims())
  {
    for (size_t d=m_dimWidgets.size(); d<m_ws->getNumDims(); d++)
    {
      DimensionSliceWidget * widget = new DimensionSliceWidget(this);
      ui.verticalLayoutDimensions->addWidget(widget);
      m_dimWidgets.push_back(widget);
      // Slot when t
      QObject::connect(widget, SIGNAL(changedShownDim(int,int,int)),
                       this, SLOT(changedShownDim(int,int,int)));
      QObject::connect(widget, SIGNAL(changedSlicePoint(int,double)),
                       this, SLOT(updateDisplaySlot(int,double)));

    }
  }
  // Set each dimension
  for (size_t d=0; d<m_ws->getNumDims(); d++)
  {
    DimensionSliceWidget * widget = m_dimWidgets[d];
    widget->setDimension( int(d), m_ws->getDimension(d) );
    // Default slicing layout
    widget->setShownDim( d < 2 ? int(d) : -1 );
  }
}


//------------------------------------------------------------------------------------
/** Set the displayed workspace. Updates UI.
 *
 * @param ws :: IMDWorkspace to show.
 */
void SliceViewer::setWorkspace(Mantid::API::IMDWorkspace_sptr ws)
{
  m_ws = ws;
  this->updateDimensionSliceWidgets();
  m_data->setWorkspace(ws);
  this->updateDisplay();
}


void SliceViewer::initLayout()
{
}
