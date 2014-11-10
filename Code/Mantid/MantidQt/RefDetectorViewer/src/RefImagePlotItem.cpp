#include "MantidQtRefDetectorViewer/RefImagePlotItem.h"

namespace MantidQt
{
namespace RefDetectorViewer
{

RefImagePlotItem::RefImagePlotItem(const RefLimitsHandler * const limitsHandler)
  : SpectrumView::SpectrumPlotItem(), m_limitsHandler(limitsHandler)
{
}


RefImagePlotItem::~RefImagePlotItem()
{
  delete m_limitsHandler;
}


/**
 *  Draw the image (this is called by QWT and must not be called directly.)
 *  Extends the base class (SpectrumPlotItem) method to draw lines on the plot
 *
 *  @param  painter     The QPainter object used by QWT to draw the image
 *  @param  xMap        The QwtScaleMap used by QWT to map x-values to pixel
 *                      columns in the actual displayed image
 *  @param  yMap        The QwtScaleMap used by QWT to map y-values to pixel
 *                      rows in the actual displayed image
 *  @param  canvasRect  rectangle containing the pixel region where QWT will
 *                      draw the image.  This rectangle is slightly larger
 *                      than the actual rectangle used for the image.  This
 *                      parameter is NOT USED by the SpectrumPlotItem, but is
 *                      passed in when QWT calls this method.
 */
void RefImagePlotItem::draw( QPainter    * painter,
                       const QwtScaleMap & xMap,
                       const QwtScaleMap & yMap,
                       const QRect       & canvasRect) const
{
  SpectrumPlotItem::draw(painter,xMap,yMap,canvasRect);

  //////////////////////////////////////////////////////////////////////////////////
  // TODO: Eliminate the code duplication (from SpectrumPlotItem::draw) in this section
  SpectrumView::DataArray_const_sptr data_array;
  if ( m_bufferID == 0 )
    data_array = m_dataArray0;
  else
    data_array = m_dataArray1;

  const double x_min  = data_array->getXMin();
  const double x_max  = data_array->getXMax();
  const double y_min  = data_array->getYMin();
  const double y_max  = data_array->getYMax();

                                            // find the actual plot region
                                            // using the scale maps.
  const int pix_x_min = (int)xMap.transform( x_min );
  const int pix_x_max = (int)xMap.transform( x_max );
  const int pix_y_min = (int)yMap.transform( y_min );
  const int pix_y_max = (int)yMap.transform( y_max );
  // End duplicated code
  //////////////////////////////////////////////////////////////////////////////////

  float coeff_left = (float(y_max) - float(y_min)) / (float(pix_y_min) - float(pix_y_max));
  float coeff_top_right;
  int pixel_value;

  //for the peak selection
  painter->setPen(Qt::blue);

  //peak1
  float peakLeft = float(m_limitsHandler->getPeakLeft());

  if (peakLeft != 0) {
    coeff_top_right = float(y_max) - float(peakLeft);
    pixel_value = int((coeff_top_right / coeff_left) + float(pix_y_max));
    painter->drawLine(QPoint(pix_x_min,pixel_value), QPoint(pix_x_max,pixel_value));
  }

  //peak2
  float peakRight = float(m_limitsHandler->getPeakRight());
  if (peakRight != 0) {
    coeff_top_right = float(y_max) - float(peakRight);
    pixel_value = int((coeff_top_right / coeff_left) + float(pix_y_max));
    painter->drawLine(QPoint(pix_x_min,pixel_value), QPoint(pix_x_max,pixel_value));
  }

  //for the background selection
  painter->setPen(Qt::red);

  //back1
  float backLeft = float(m_limitsHandler->getBackLeft());
  if (backLeft != 0) {
    coeff_top_right = float(y_max) - float(backLeft);
    pixel_value = int((coeff_top_right / coeff_left) + float(pix_y_max));
    painter->drawLine(QPoint(pix_x_min,pixel_value), QPoint(pix_x_max,pixel_value));
  }

  //back2
  float backRight = float(m_limitsHandler->getBackRight());
  if (backRight != 0) {
    coeff_top_right = float(y_max) - float(backRight);
    pixel_value = int((coeff_top_right / coeff_left) + float(pix_y_max));
    painter->drawLine(QPoint(pix_x_min,pixel_value), QPoint(pix_x_max,pixel_value));
  }

  //tof selection
  painter->setPen(Qt::green);

  coeff_left = (float(pix_x_max) - float(pix_x_min)) / (float(x_max) - float(x_min));
  int tof_value;
  float coeff_bottom_right;

  //tof min
  float TOFmin = float(m_limitsHandler->getTOFmin());
  if (TOFmin != 0) {
    coeff_bottom_right = float(TOFmin) - float(x_min);
    tof_value = int(coeff_left * coeff_bottom_right + static_cast<float>(pix_x_min));
    painter->drawLine(QPoint(tof_value,pix_y_min), QPoint(tof_value,pix_y_max));
  }

  //tof max
  float TOFmax = float(m_limitsHandler->getTOFmax());
  if (TOFmax != 0) {
    coeff_bottom_right = float(TOFmax) - float(x_min);
    tof_value = int(coeff_left * coeff_bottom_right + float(pix_x_min));
    painter->drawLine(QPoint(tof_value,pix_y_min), QPoint(tof_value,pix_y_max));
  }
}

} // namespace RefDetectorViewer
} // namespace MantidQt
