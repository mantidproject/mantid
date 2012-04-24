
#include <iostream>
#include <QThread>
#include "MantidQtImageViewer/ImagePlotItem.h"

namespace MantidQt
{
namespace ImageView
{
  

/**
 * Construct basic plot item with NO data to plot.
 */
ImagePlotItem::ImagePlotItem()
{
  data_array      = 0;
  color_table     = 0;
  intensity_table = 0;
}


ImagePlotItem::~ImagePlotItem()
{
}


/**
 * Specify the data to be plotted and the color table to use.
 *
 * @param data_array   The DataArray object containing the data to plot
 *                     along with information about the array size and 
 *                     the region covered by the data. 
 * @param color_table  Vector of RGB colors that determine the mapping 
 *                     from a data value to a color. 
 */
void ImagePlotItem::SetData( DataArray*         data_array, 
                             std::vector<QRgb>* color_table )
{
  this->data_array  = data_array;
  this->color_table = color_table;
}


/**
 *  Set a non-linear look up table that will be used with data values before
 *  they are mapped to a color.  This is typically used to apply a log type
 *  scaling so lower level values can be seen better.
 *
 *  @param intensity_table  Look up table containing values between 0 and 1
 *                          that will be used to scale the corresponding
 *                          image values before mappign to a color index.
 */
void ImagePlotItem::SetIntensityTable( std::vector<double>* intensity_table )
{
  this->intensity_table = intensity_table;
}


/**
 *  Draw the image (this is called by QWT and must not be called directly.)
 *
 *  @param  painter     The QPainter object used by QWT to draw the image
 *  @param  xMap        The QwtScaleMap used by QWT to map x-values to pixel
 *                      columns in the actual displayed image
 *  @param  yMap        The QwtScaleMap used by QWT to map y-values to pixel
 *                      rows in the actual displayed image
 *  @param  canvasRect  rectangle containing the pixel region where QWT will 
 *                      draw the image.  This rectangle is slightly larger
 *                      than the actual rectangle used for the image.  This
 *                      parameter is NOT USED by the ImagePlotItem, but is
 *                      passed in when QWT calls this method. 
 */
void ImagePlotItem::draw(       QPainter    * painter,
                          const QwtScaleMap & xMap, 
                          const QwtScaleMap & yMap,
                          const QRect       &       ) const
{
  if ( !data_array || !color_table )     // if data not yet set, just return
  {
    return;
  }

  size_t n_rows = data_array->GetNRows();
  size_t n_cols = data_array->GetNCols();

  double min    = data_array->GetDataMin();
  double max    = data_array->GetDataMax();
  double x_min  = data_array->GetXMin();
  double x_max  = data_array->GetXMax();
  double y_min  = data_array->GetYMin();
  double y_max  = data_array->GetYMax();

  float *data   = data_array->GetData();
                                            // find the actual plot region
                                            // using the scale maps. 
  int pix_x_min = (int)xMap.transform( x_min );
  int pix_x_max = (int)xMap.transform( x_max );
  int pix_y_min = (int)yMap.transform( y_min );
  int pix_y_max = (int)yMap.transform( y_max );

  double scale = ((double)color_table->size()-1)/(max-min);
  size_t lut_size = 0;
  double ct_scale = ((double)color_table->size()-1);
  if ( intensity_table != 0 )
  {
    lut_size = intensity_table->size();
    scale    = ((double)lut_size-1.0) / (max-min);
    ct_scale = ((double)color_table->size()-1);
  }
  double shift = -min * scale;
  size_t data_index;
  size_t color_index;
  size_t lut_index;
  size_t image_index = 0;

  unsigned int* rgb_buffer = new unsigned int[n_rows * n_cols];

  for ( int row = (int)n_rows-1; row >= 0; row-- )
  {
    data_index = row * n_cols;
    if (intensity_table == 0 )              // use color table directly
    {
      for ( int col = 0; col < (int)n_cols; col++ )
      {
        color_index = (uint)(data[data_index] * scale + shift);
        rgb_buffer[image_index] = (*color_table)[ color_index ];
        image_index++;
        data_index++;
      }
    }
    else                                    // go through intensity table
    {
      for ( int col = 0; col < (int)n_cols; col++ )
      {
        lut_index   = (uint)(data[data_index] * scale + shift);
        color_index = (uint)((*intensity_table)[lut_index] * ct_scale );
        rgb_buffer[image_index] = (*color_table)[ color_index ];
        image_index++;
        data_index++;
      }
    }
  }

  uchar *buffer = (uchar*)rgb_buffer;
  QImage image( buffer, (int)n_cols, (int)n_rows, QImage::Format_RGB32 );
  QPixmap pixmap = QPixmap::fromImage(image);

  int width  = pix_x_max - pix_x_min + 1;
  int height = pix_y_min - pix_y_max + 1;    // y-axis is inverted for image

  QPixmap scaled_pixmap = pixmap.scaled( width, height, 
                                         Qt::IgnoreAspectRatio,
                                         Qt::FastTransformation);

  painter->drawPixmap( pix_x_min, pix_y_max, scaled_pixmap );

  delete[] rgb_buffer;                       // hopefully we can delete this
                                             // now  
}


} // namespace MantidQt 
} // namespace ImageView 
