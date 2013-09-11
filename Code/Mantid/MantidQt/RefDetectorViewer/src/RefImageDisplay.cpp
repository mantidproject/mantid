#include "MantidQtRefDetectorViewer/RefImageDisplay.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtSpectrumViewer/IVUtils.h"
#include "MantidQtRefDetectorViewer/RefImagePlotItem.h"

namespace MantidQt
{
namespace RefDetectorViewer
{
  using namespace SpectrumView;

/**
 * Make an RefImageDisplay to display with the given widgets and controls.
 *
 * @param image_plot      The QwtPlot that will hold the image
 * @param slider_handler  The object that manages interaction with the
 *                        horizontal and vertical scroll bars
 * @param range_handler   The object that manages the data range
 * @param limits_handler  The object that manages the limits
 * @param h_graph         The GraphDisplay for the graph showing horizontal
 *                        cuts through the image at the bottom of the image.
 * @param v_graph         The GraphDisplay for the graph showing vertical 
 *                        cuts through the image at the left side of the image.
 * @param table_widget    The widget where the information about a pointed
 *                        at location will be displayed.
 */
RefImageDisplay::RefImageDisplay(  QwtPlot*       image_plot,
                             RefSliderHandler* slider_handler,
                             RefRangeHandler*  range_handler,
                             RefLimitsHandler* limits_handler,
                             GraphDisplay*  h_graph,
                             GraphDisplay*  v_graph,
                             QTableWidget*  table_widget)
  : SpectrumView::SpectrumDisplay(image_plot,slider_handler,range_handler,h_graph,v_graph,table_widget),
    m_limitsHandler(limits_handler)
{
  // We need a different SpectrumPlotItem class, so delete the one created in the
  // base class constructor and create the one we want
  delete image_plot_item;
  image_plot_item = new RefImagePlotItem(limits_handler);
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
QPair<double,double> RefImageDisplay::SetPointedAtPoint( QPoint point, int mouseClick)
{
  // Call the base class method for most of the work
  QPair<double,double> xy = SpectrumDisplay::SetPointedAtPoint( point, mouseClick );

  // Now, for a left click, set the position in the appropriate lineedit
  if (mouseClick == 1)  //left click
  {
    m_limitsHandler->setActiveValue( xy.first, xy.second );
    UpdateImage(); //force refresh of the plot
  }
  
  return xy;
}

} // namespace RefDetectorViewer
} // namespace MantidQt 
