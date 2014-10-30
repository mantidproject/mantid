#include "MantidQtRefDetectorViewer/RefImageDisplay.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtRefDetectorViewer/RefImagePlotItem.h"

namespace MantidQt
{
namespace RefDetectorViewer
{
  using namespace SpectrumView;

/**
 * Make a RefImageDisplay to display with the given widgets and controls.
 *
 * @param imagePlot      The QwtPlot that will hold the image
 * @param sliderHandler  The object that manages interaction with the
 *                       horizontal and vertical scroll bars
 * @param rangeHandler   The object that manages the data range
 * @param limitsHandler  The object that manages the limits
 * @param hGraph         The GraphDisplay for the graph showing horizontal
 *                       cuts through the image at the bottom of the image.
 * @param vGraph         The GraphDisplay for the graph showing vertical
 *                       cuts through the image at the left side of the image.
 * @param tableWidget    The widget where the information about a pointed
 *                       at location will be displayed.
 */
RefImageDisplay::RefImageDisplay( QwtPlot*          imagePlot,
                                  RefSliderHandler* sliderHandler,
                                  RefRangeHandler*  rangeHandler,
                                  RefLimitsHandler* limitsHandler,
                                  GraphDisplay*     hGraph,
                                  GraphDisplay*     vGraph,
                                  QTableWidget*     tableWidget)
  : SpectrumView::SpectrumDisplay(imagePlot, sliderHandler, rangeHandler, hGraph, vGraph, tableWidget),
    m_limitsHandler(limitsHandler)
{
  // We need a different SpectrumPlotItem class, so delete the one created in the
  // base class constructor and create the one we want
  delete m_spectrumPlotItem;
  m_spectrumPlotItem = new RefImagePlotItem(limitsHandler);
  setupSpectrumPlotItem();
}

RefImageDisplay::~RefImageDisplay()
{
}

/**
 * Extract data from horizontal and vertical cuts across the image and
 * show those as graphs in the horizontal and vertical graphs and show
 * information about the specified point.
 *
 * @param point  The point that the user is currently pointing at with
 *               the mouse.
 * @param mouseClick Which mouse button was clicked
 * @return A pair containing the (x,y) values in the graph of the point
 */
QPair<double,double> RefImageDisplay::setPointedAtPoint( QPoint point, int mouseClick)
{
  // Call the base class method for most of the work
  QPair<double,double> xy = SpectrumDisplay::setPointedAtPoint( point, mouseClick );

  // Now, for a left click, set the position in the appropriate lineedit
  if (mouseClick == 1)  //left click
  {
    m_limitsHandler->setActiveValue( xy.first, xy.second );
    updateImage(); //force refresh of the plot
  }

  return xy;
}

} // namespace RefDetectorViewer
} // namespace MantidQt
