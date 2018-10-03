// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/RefDetectorView/RefImageDisplay.h"
#include "MantidQtWidgets/RefDetectorView/RefImagePlotItem.h"
#include "MantidQtWidgets/SpectrumViewer/ColorMaps.h"
#include "MantidQtWidgets/SpectrumViewer/DataArray.h"
#include "MantidQtWidgets/SpectrumViewer/QtUtils.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"

namespace MantidQt {
namespace RefDetectorViewer {
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
RefImageDisplay::RefImageDisplay(QwtPlot *imagePlot,
                                 RefSliderHandler *sliderHandler,
                                 RefRangeHandler *rangeHandler,
                                 RefLimitsHandler *limitsHandler,
                                 GraphDisplay *hGraph, GraphDisplay *vGraph,
                                 QTableWidget *tableWidget)
    : SpectrumView::SpectrumDisplay(imagePlot, sliderHandler, rangeHandler,
                                    hGraph, vGraph, tableWidget),
      m_limitsHandler(limitsHandler) {
  // We need a different SpectrumPlotItem class, so delete the one created in
  // the
  // base class constructor and create the one we want
  delete m_spectrumPlotItem;
  m_spectrumPlotItem = new RefImagePlotItem(limitsHandler);
  setupSpectrumPlotItem();
}

RefImageDisplay::~RefImageDisplay() {}

/**
 * Extract data from horizontal and vertical cuts across the image and
 * show those as graphs in the horizontal and vertical graphs and show
 * information about the specified point.
 *
 * @param point  The point that the user is currently pointing at with
 *               the mouse.
 * @param mouseClick Which mouse button was clicked
 * @param isFront Flag indicating if this is a call to the front (active)
 *display.
 * @return A pair containing the (x,y) values in the graph of the point
 */
QPair<double, double>
RefImageDisplay::setPointedAtPoint(QPoint point, int mouseClick, bool isFront) {
  UNUSED_ARG(isFront);
  // Call the base class method for most of the work
  QPair<double, double> xy = SpectrumDisplay::setPointedAtPoint(point);

  // Now, for a left click, set the position in the appropriate lineedit
  if (mouseClick == 1) // left click
  {
    m_limitsHandler->setActiveValue(xy.first, xy.second);
    updateImage(); // force refresh of the plot
  }

  return xy;
}

} // namespace RefDetectorViewer
} // namespace MantidQt
