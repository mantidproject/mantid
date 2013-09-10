
#include <iostream>
#include <QThread>
#include "MantidQtSpectrumViewer/ImagePlotItem.h"

namespace MantidQt
{
namespace ImageView
{
  

/**
 * Construct basic plot item with NO data to plot.
 */
ImagePlotItem::ImagePlotItem()
{
  buffer_ID            = 0;
  data_array_0         = 0;
  data_array_1         = 0;
  positive_color_table = 0;
  negative_color_table = 0;
  intensity_table      = 0;
}


ImagePlotItem::~ImagePlotItem()
{
  //std::cout << "ImagePlotItem destructor called" << std::endl;

  if ( data_array_0 )
  {
    delete data_array_0; 
  }
  if ( data_array_1 )
  {
    delete data_array_1; 
  }
}


/**
 * Specify the data to be plotted and the color table to use.
 *
 * @param data_array            The DataArray object containing the data to 
 *                              plot along with information about the array  
 *                              size and the region covered by the data. 
 * @param positive_color_table  Vector of RGB colors that determine the mapping 
 *                              from a positive data value to a color. 
 * @param negative_color_table  Vector of RGB colors that determine the mapping 
 *                              from a negative data value to a color. This
 *                              must have the same number of entries as the
 *                              positive color table.
 */
void ImagePlotItem::SetData( DataArray*         data_array, 
                             std::vector<QRgb>* positive_color_table,
                             std::vector<QRgb>* negative_color_table )
{
  if ( buffer_ID == 0 )
  {
    if ( data_array_1 )         // we must be done using array 1, so delete it
    {
      delete data_array_1; 
    }
    data_array_1 = data_array;  // put new data in array 1, and switch to it
                                // leaving array 0 intact for now, in case it's
                                // being drawn.
    buffer_ID = 1;
  }
  else
  {
    if ( data_array_0 )         // we must be done using array 0, so delete it
    {
      delete data_array_0;
    }
    data_array_0 = data_array;  // put new data in array 0, and switch to it
                                // leaving array 1 intact for now, in case it's
                                // being drawn.
    buffer_ID = 0;
  }
  this->positive_color_table = positive_color_table;
  this->negative_color_table = negative_color_table;
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
  if ( !positive_color_table )     // if no color table, the data is not yet
  {                                // set, so just return
    return;
  }

  DataArray* data_array;
  if ( buffer_ID == 0 )
  {
    data_array = data_array_0;
  }
  else
  {
    data_array = data_array_1;
  }

  size_t n_rows = data_array->GetNRows();
  size_t n_cols = data_array->GetNCols();

  if ( n_rows <= 0 || n_cols <= 0 )
  {
    return;                                 // can't draw degenerate image
  }

  const double min    = data_array->GetDataMin();
  const double max    = data_array->GetDataMax();
  const double x_min  = data_array->GetXMin();
  const double x_max  = data_array->GetXMax();
  const double y_min  = data_array->GetYMin();
  const double y_max  = data_array->GetYMax();

  float *data   = data_array->GetData();
                                            // find the actual plot region
                                            // using the scale maps. 
  const int pix_x_min = (int)xMap.transform( x_min );
  const int pix_x_max = (int)xMap.transform( x_max );
  const int pix_y_min = (int)yMap.transform( y_min );
  const int pix_y_max = (int)yMap.transform( y_max );

                                            // set up zero centered scale range
                                            // symmetrical around zero
  double zc_max = fabs(max);
  if ( fabs(min) > fabs(max) )
  {
    zc_max = fabs(min);
  }
  if ( zc_max == 0 )                        // all values are zero, set up
  {                                         // no-degenerate default range
    zc_max = 1;
  }

  double scale = ((double)positive_color_table->size()-1)/zc_max;
  size_t lut_size = 0;
  double ct_scale = ((double)positive_color_table->size()-1);
  if ( intensity_table != 0 )
  {
    lut_size = intensity_table->size();
    scale    = ((double)lut_size-1.0) / zc_max;
    ct_scale = ((double)positive_color_table->size()-1);
  }
  size_t data_index;
  size_t color_index;
  size_t lut_index;
  size_t image_index = 0;

  unsigned int* rgb_buffer = new unsigned int[n_rows * n_cols];
  double val = 0;
  for ( int row = (int)n_rows-1; row >= 0; row-- )
  {
    data_index = row * n_cols;
    if (intensity_table == 0 )              // use color tables directly
    {
      for ( int col = 0; col < (int)n_cols; col++ )
      {
        val = data[data_index] * scale; 
        if ( val >= 0 )                     // use positive color table
        {
          color_index = (uint)val;
          rgb_buffer[image_index] = (*positive_color_table)[ color_index ];
        }
        else                               // use negative color table
        {
          color_index = (uint)(-val);
          rgb_buffer[image_index] = (*negative_color_table)[ color_index ];
        }
        image_index++;
        data_index++;
      }
    }
    else                                    // go through intensity table
    {
      for ( int col = 0; col < (int)n_cols; col++ )
      {
        val = data[data_index] * scale;
        if ( val >= 0 )
        {
          lut_index   = (uint)val;
          color_index = (uint)((*intensity_table)[lut_index] * ct_scale );
          rgb_buffer[image_index] = (*positive_color_table)[ color_index ];
        }
        else
        {
          lut_index   = (uint)(-val);
          color_index = (uint)((*intensity_table)[lut_index] * ct_scale );
          rgb_buffer[image_index] = (*negative_color_table)[ color_index ];
        }
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

  delete[] rgb_buffer;                       // we can delete this now, but
                                             // not earlier since the image
                                             // and/or pixmap is using it
}


} // namespace MantidQt 
} // namespace ImageView 
