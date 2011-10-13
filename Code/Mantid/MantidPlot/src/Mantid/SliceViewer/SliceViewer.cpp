#include "CustomTools.h"
#include "DimensionSliceWidget.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/VMD.h"
#include "QwtRasterDataMD.h"
#include "SliceViewer.h"
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <qwt_color_map.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot.h>
#include <qwt_scale_engine.h>
#include <sstream>
#include <vector>
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using Mantid::Kernel::VMD;


SliceViewer::SliceViewer(QWidget *parent)
    : QWidget(parent),
      m_dimX(0), m_dimY(0),
      m_logColor(false)
{
	ui.setupUi(this);


	// Create the plot
  m_spectLayout = new QHBoxLayout(ui.frmPlot);
	m_plot = new QwtPlot();
  m_plot->autoRefresh();
  m_spectLayout->addWidget(m_plot, 1, 0);

	// Add a spectrograph
	m_spect = new QwtPlotSpectrogram();
	m_spect->attach(m_plot);

	m_colorMap = QwtLinearColorMap(Qt::blue, Qt::red);
	QwtDoubleInterval range(0.0, 10.0);

	m_data = new QwtRasterDataMD();
	m_spect->setColorMap(m_colorMap);
  m_plot->autoRefresh();

  // --- Create a color bar on the right axis ---------------
  m_colorBar = m_plot->axisWidget(QwtPlot::yRight);
  m_colorBar->setColorBarEnabled(true);
  m_colorBar->setColorMap(range, m_colorMap);
  m_plot->setAxisScale(QwtPlot::yRight, range.minValue(), range.maxValue() );
  m_plot->enableAxis(QwtPlot::yRight);

  // Make the splitter use the minimum size for the controls and not stretch out
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 1);
  initZoomer();

  // ----------- Toolbar button signals ----------------
  QObject::connect(ui.btnResetZoom, SIGNAL(clicked()), this, SLOT(resetZoom()));
  QObject::connect(ui.btnRangeFull, SIGNAL(clicked()), this, SLOT(colorRangeFullSlot()));
  QObject::connect(ui.btnRangeSlice, SIGNAL(clicked()), this, SLOT(colorRangeSliceSlot()));
  ui.btnZoom->hide();

}

SliceViewer::~SliceViewer()
{
  delete m_data;
  // Don't delete Qt objects, I think these are auto-deleted
}

//------------------------------------------------------------------------------------
/** Intialize the zooming/panning tools */
void SliceViewer::initZoomer()
{
//  QwtPlotZoomer * zoomer = new CustomZoomer(m_plot->canvas());
//  zoomer->setMousePattern(QwtEventPattern::MouseSelect1,  Qt::LeftButton, Qt::ControlModifier);
//  zoomer->setTrackerMode(QwtPicker::AlwaysOn);
//  const QColor c(Qt::darkBlue);
//  zoomer->setRubberBandPen(c);
//  zoomer->setTrackerPen(c);

  // Zoom in/out using right-click or the mouse wheel
  QwtPlotMagnifier * magnif = new CustomMagnifier(m_plot->canvas());
  magnif->setAxisEnabled(QwtPlot::yRight, false); // Don't do the colorbar axis
  magnif->setWheelFactor(0.9);
  // Have to flip the keys to match our flipped mouse wheel
  magnif->setZoomInKey(Qt::Key_Minus, Qt::NoModifier);
  magnif->setZoomOutKey(Qt::Key_Equal, Qt::NoModifier);

  // Pan using the middle button
  QwtPlotPanner *panner = new QwtPlotPanner(m_plot->canvas());
  panner->setMouseButton(Qt::MidButton);
  panner->setAxisEnabled(QwtPlot::yRight, false); // Don't do the colorbar axis

  CustomPicker * picker = new CustomPicker(m_spect->xAxis(), m_spect->yAxis(), m_plot->canvas());
  QObject::connect(picker, SIGNAL(mouseMoved(double,double)), this, SLOT(showInfoAt(double, double)));

}


//------------------------------------------------------------------------------------
/** Programmatically show/hide the controls (sliders etc)
 *
 * @param visible :: true if you want to show the controls.
 */
void SliceViewer::showControls(bool visible)
{
  ui.frmControls->setVisible(visible);
}


//------------------------------------------------------------------------------------
/** Reset the axis and scale it
 *
 * @param axis :: int for X or Y
 * @param dim :: dimension to show
 */
void SliceViewer::resetAxis(int axis, Mantid::Geometry::IMDDimension_const_sptr dim)
{
  m_plot->setAxisScale( axis, dim->getMinimum(), dim->getMaximum(), (dim->getMaximum()-dim->getMinimum())/5);
  m_plot->setAxisTitle( axis, QString::fromStdString(dim->getName() + " (" + dim->getUnits() + ")") );
}


//------------------------------------------------------------------------------------
/// Reset the zoom view to full axes. This can be called manually with a button
void SliceViewer::resetZoom()
{
  // Reset the 2 axes to full scale
  resetAxis(m_spect->xAxis(), m_X );
  resetAxis(m_spect->yAxis(), m_Y );
  // Make sure the view updates
  m_plot->replot();
}


//------------------------------------------------------------------------------------
/** Get the range of signal given an iterator
 *
 * @param it :: IMDIterator of what to find
 * @return the min/max range, or 0-1.0 if not found
 */
QwtDoubleInterval getRange(IMDIterator * it)
{
  if (!it)
    return QwtDoubleInterval(0., 1.0);
  if (!it->valid())
    return QwtDoubleInterval(0., 1.0);

  double minSignal = DBL_MAX;
  double maxSignal = -DBL_MAX;
  do
  {
    double signal = it->getNormalizedSignal();
    if (signal < minSignal) minSignal = signal;
    if (signal > maxSignal) maxSignal = signal;
  } while (it->next());

  if (minSignal < maxSignal)
    return QwtDoubleInterval(minSignal, maxSignal);
  else
    // Possibly only one value in range
    return QwtDoubleInterval(minSignal-0.5, minSignal+0.5);
}

//------------------------------------------------------------------------------------
/// Find the full range of values in the workspace
void SliceViewer::findRangeFull()
{
  if (!m_ws) return;
  // Iterate through the entire workspace
  IMDIterator * it = m_ws->createIterator();
  m_colorRangeFull = getRange(it);
  delete it;
}


//------------------------------------------------------------------------------------
/** Find the full range of values ONLY in the currently visible
part of the workspace */
void SliceViewer::findRangeSlice()
{
  if (!m_ws) return;
  m_colorRangeSlice = QwtDoubleInterval(0., 1.0);

  // This is what is currently visible on screen
  QwtDoubleInterval xint = m_plot->axisScaleDiv( m_spect->xAxis() )->interval();
  QwtDoubleInterval yint = m_plot->axisScaleDiv( m_spect->yAxis() )->interval();

  // Find the min-max extents in each dimension
  VMD min(m_ws->getNumDims());
  VMD max(m_ws->getNumDims());
  for (size_t d=0; d<m_ws->getNumDims(); d++)
  {
    DimensionSliceWidget * widget = m_dimWidgets[int(d)];
    IMDDimension_const_sptr dim = m_ws->getDimension(d);
    if (widget->getShownDim() == 0)
    {
      min[d] = xint.minValue();
      max[d] = xint.maxValue();
    }
    else if (widget->getShownDim() == 1)
    {
      min[d] = yint.minValue();
      max[d] = yint.maxValue();
    }
    else
    {
      // Is a slice. Take a slice of widht = binWidth
      min[d] = widget->getSlicePoint() - dim->getBinWidth() * 0.45;
      max[d] = min[d] + dim->getBinWidth();
    }
  }
  // This builds the implicit function for just this slice
  MDBoxImplicitFunction * function = new MDBoxImplicitFunction(min, max);

  // Iterate through the slice
  IMDIterator * it = m_ws->createIterator(function);
  m_colorRangeSlice = getRange(it);
  // In case of failure, use the full range instead
  if (m_colorRangeSlice == QwtDoubleInterval(0.0, 1.0))
    m_colorRangeSlice = m_colorRangeFull;
  delete it;
}


//------------------------------------------------------------------------------------
/// Slot for finding the data full range and updating the display
void SliceViewer::colorRangeFullSlot()
{
  this->findRangeFull();
  m_colorRange = m_colorRangeFull;
  this->updateDisplay();
}

//------------------------------------------------------------------------------------
/// Slot for finding the current view/slice full range and updating the display
void SliceViewer::colorRangeSliceSlot()
{
  this->findRangeSlice();
  m_colorRange = m_colorRangeSlice;
  this->updateDisplay();
}





//------------------------------------------------------------------------------------
void SliceViewer::showInfoAt(double x, double y)
{
  if (!m_ws) return;
  VMD coords(m_ws->getNumDims());
  for (size_t d=0; d<m_ws->getNumDims(); d++)
    coords[d] = m_dimWidgets[int(d)]->getSlicePoint();
  coords[m_dimX] = x;
  coords[m_dimY] = y;
  signal_t signal = m_ws->getSignalAtCoord(coords);
  ui.lblInfoX->setText(QString::number(x, 'g', 4));
  ui.lblInfoY->setText(QString::number(y, 'g', 4));
  ui.lblInfoSignal->setText(QString::number(signal, 'g', 4));
}

//------------------------------------------------------------------------------------
/** Update the 2D plot using all the current controls settings */
void SliceViewer::updateDisplay()
{
  m_data->timesRequested = 0;
  size_t oldX = m_dimX;
  size_t oldY = m_dimY;

  m_dimX = 0;
  m_dimY = 1;
  std::vector<coord_t> slicePoint;
  for (size_t d=0; d<m_ws->getNumDims(); d++)
  {
    DimensionSliceWidget * widget = m_dimWidgets[int(d)];
    if (widget->getShownDim() == 0)
      m_dimX = d;
    if (widget->getShownDim() == 1)
      m_dimY = d;
    slicePoint.push_back(widget->getSlicePoint());
  }
  // Avoid going out of range
  if (m_dimX >= m_ws->getNumDims()) m_dimX = m_ws->getNumDims()-1;
  if (m_dimY >= m_ws->getNumDims()) m_dimY = m_ws->getNumDims()-1;
  m_data->setSliceParams(m_dimX, m_dimY, slicePoint);

  m_X = m_ws->getDimension(m_dimX);
  m_Y = m_ws->getDimension(m_dimY);

  // Was there a change of which dimensions are shown?
  if (oldX != m_dimX || oldY != m_dimY )
  {
    this->resetAxis(m_spect->xAxis(), m_X );
    this->resetAxis(m_spect->yAxis(), m_Y );
  }

  // Set the color range
  m_data->setRange(m_colorRange);
  m_colorBar->setColorMap(m_colorRange, m_colorMap);
  m_plot->setAxisScale(QwtPlot::yRight, m_colorRange.minValue(), m_colorRange.maxValue() );

  // Notify the graph that the underlying data changed
  m_spect->setData(*m_data);
  m_spect->itemChanged();
  m_plot->replot();
//  std::cout << m_plot->sizeHint().width() << " width\n";
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
            (m_dimWidgets[int(d)]->getShownDim() == dim))
        {
          // So flip it. If the new one is X, the old one becomes Y
          m_dimWidgets[int(d)]->setShownDim( (dim==0) ? 1 : 0 );
          break;
        }
      }
    }
    // Make sure no other dimension is showing the same one
    for (size_t d=0; d<m_ws->getNumDims(); d++)
    {
      // A different dimension had the same shown dimension
      if ((size_t(index) != d) &&
          (m_dimWidgets[int(d)]->getShownDim() == dim))
      {
        m_dimWidgets[int(d)]->setShownDim(-1);
      }
    }
  }
  this->updateDisplay();
}


//------------------------------------------------------------------------------------
/** Slot to redraw when the slice point changes */
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
  if (m_dimWidgets.size() < int(m_ws->getNumDims()))
  {
    for (size_t d=m_dimWidgets.size(); d<m_ws->getNumDims(); d++)
    {
      DimensionSliceWidget * widget = new DimensionSliceWidget(this);
      ui.verticalLayoutControls->insertWidget(int(d), widget);
      m_dimWidgets.push_back(widget);
      // Slot when t
      QObject::connect(widget, SIGNAL(changedShownDim(int,int,int)),
                       this, SLOT(changedShownDim(int,int,int)));
      QObject::connect(widget, SIGNAL(changedSlicePoint(int,double)),
                       this, SLOT(updateDisplaySlot(int,double)));
    }
  }

  int maxLabelWidth = 10;
  int maxUnitsWidth = 10;
  // Set each dimension
  for (size_t d=0; d<m_ws->getNumDims(); d++)
  {
    DimensionSliceWidget * widget = m_dimWidgets[int(d)];
    widget->setDimension( int(d), m_ws->getDimension(d) );
    // Default slicing layout
    widget->setShownDim( d < 2 ? int(d) : -1 );
    // To harmonize the layout, find the largest label
    int w;
    w = widget->ui.lblName->sizeHint().width();
    if (w > maxLabelWidth) maxLabelWidth = w;
    w = widget->ui.lblUnits->sizeHint().width();
    if (w > maxUnitsWidth) maxUnitsWidth = w;
  }

  // Make the labels all the same width
  for (size_t d=0; d<m_ws->getNumDims(); d++)
  {
    DimensionSliceWidget * widget = m_dimWidgets[int(d)];
    widget->ui.lblName->setMinimumSize(QSize(maxLabelWidth, 0) );
    widget->ui.lblUnits->setMinimumSize(QSize(maxUnitsWidth, 0) );
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
  // Find the full range. And use it
  findRangeFull();
  m_colorRange = m_colorRangeFull;
  // Initial display update
  this->updateDisplay();
}


