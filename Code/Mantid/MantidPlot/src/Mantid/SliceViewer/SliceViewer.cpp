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


//------------------------------------------------------------------------------------
/** Update the 2D plot using all the current controls settings */
void SliceViewer::updateDisplay()
{
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

//  m_data->setBoundingRect( QwtDoubleRect(m_X->getMinimum(), m_X->getMaximum(), m_Y->getMinimum(), m_Y->getMaximum()) );

  // Notify the graph that the underlying data changed
  //m_spect->setAxisScale(); // boundingRect() = QwtDoubleRect(m_X->getMinimum(), m_X->getMaximum(), m_Y->getMinimum(), m_Y->getMaximum());
  m_plot->setAxisScale( m_spect->xAxis(), m_X->getMinimum(), m_X->getMaximum(), 1.0);
  m_plot->setAxisTitle( m_spect->xAxis(), QString::fromStdString(m_X->getName() + " (" + m_X->getUnits() + ")") );
  m_plot->setAxisScale( m_spect->yAxis(), m_Y->getMinimum(), m_Y->getMaximum(), 1.0);
  m_plot->setAxisTitle( m_spect->yAxis(), QString::fromStdString(m_Y->getName() + " (" + m_Y->getUnits() + ")") );
//  m_plot->axisScaleDraw( m_spect->xAxis() )->setMinimumExtent(10);
//  m_plot->axisScaleDraw( m_spect->yAxis() )->setMinimumExtent(10);
  m_spect->setData(*m_data);
  m_spect->itemChanged();
  m_plot->replot();
//  m_plot->setSizePolicy( Qt::MinimumExpanding );
//  m_plot->resize(100,100);
  std::cout << m_plot->sizeHint().width() << " width\n";

}



//------------------------------------------------------------------------------------
/** The user changed the shown dimension somewhere.
 *
 * @param index :: index of the dimension
 * @param dim :: shown dimension, 0=X, 1=Y, -1 sliced
 */
void SliceViewer::changedShownDim(int index, int dim)
{
  if (dim >= 0)
  {
    for (size_t d=0; d<m_ws->getNumDims(); d++)
    {
      // A different dimension had the same shown dimension
      if ((size_t(index) != d) &&
          (m_dimWidgets[d]->getShownDim() == dim))
      {
        // So flip it. If the new one is X, the old one becomes Y
        m_dimWidgets[d]->setShownDim( (dim==0) ? 1 : 0 );
      }
    }
  }
  this->updateDisplay();
}

/** Slot to redraw when the slice poitn changes */
void SliceViewer::updateDisplaySlot(int index, double value)
{
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
      QObject::connect(widget, SIGNAL(changedShownDim(int,int)),
                       this, SLOT(changedShownDim(int,int)));
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
