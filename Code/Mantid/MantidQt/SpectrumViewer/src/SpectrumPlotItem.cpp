#include <iostream>
#include <QThread>
#include "MantidQtSpectrumViewer/SpectrumPlotItem.h"

namespace MantidQt
{
namespace SpectrumView
{


/**
 * Construct basic plot item with NO data to plot.
 */
SpectrumPlotItem::SpectrumPlotItem() :
  m_bufferID(0),
  /* m_dataArray0(NULL), */
  /* m_dataArray1(NULL), */
  m_positiveColorTable(NULL),
  m_negativeColorTable(NULL),
  m_intensityTable(NULL)
{
}


SpectrumPlotItem::~SpectrumPlotItem()
{
}


/**
 * Specify the data to be plotted and the color table to use.
 *
 * @param dataArray           The DataArray object containing the data to
 *                            plot along with information about the array
 *                            size and the region covered by the data.
 * @param positiveColorTable  Vector of RGB colors that determine the mapping
 *                            from a positive data value to a color.
 * @param negativeColorTable  Vector of RGB colors that determine the mapping
 *                            from a negative data value to a color. This
 *                            must have the same number of entries as the
 *                            positive color table.
 */
void SpectrumPlotItem::setData( DataArray_const_sptr dataArray,
                                std::vector<QRgb>* positiveColorTable,
                                std::vector<QRgb>* negativeColorTable )
{
  if ( m_bufferID == 0 )
  {
    m_dataArray1 = dataArray;  // put new data in array 1, and switch to it
                               // leaving array 0 intact for now, in case it's
                               // being drawn.
    m_bufferID = 1;
  }
  else
  {
    m_dataArray0 = dataArray;  // put new data in array 0, and switch to it
                               // leaving array 1 intact for now, in case it's
                               // being drawn.
    m_bufferID = 0;
  }

  m_positiveColorTable = positiveColorTable;
  m_negativeColorTable = negativeColorTable;
}


/**
 *  Set a non-linear look up table that will be used with data values before
 *  they are mapped to a color.  This is typically used to apply a log type
 *  scaling so lower level values can be seen better.
 *
 *  @param intensityTable  Look up table containing values between 0 and 1
 *                         that will be used to scale the corresponding
 *                         image values before mappign to a color index.
 */
void SpectrumPlotItem::setIntensityTable( std::vector<double>* intensityTable )
{
  m_intensityTable = intensityTable;
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
 *                      parameter is NOT USED by the SpectrumPlotItem, but is
 *                      passed in when QWT calls this method.
 */
void SpectrumPlotItem::draw(QPainter    * painter,
                      const QwtScaleMap & xMap,
                      const QwtScaleMap & yMap,
                      const QRect       &       ) const
{
  // If no color table, the data is not yet set, so just return
  if(!m_positiveColorTable)
    return;

  DataArray_const_sptr dataArray;
  if ( m_bufferID == 0 )
    dataArray = m_dataArray0;
  else
    dataArray = m_dataArray1;

  size_t n_rows = dataArray->getNRows();
  size_t n_cols = dataArray->getNCols();

  if ( n_rows == 0 || n_cols == 0 )
  {
    return;                                 // can't draw degenerate image
  }

  const double min    = dataArray->getDataMin();
  const double max    = dataArray->getDataMax();
  const double x_min  = dataArray->getXMin();
  const double x_max  = dataArray->getXMax();
  const double y_min  = dataArray->getYMin();
  const double y_max  = dataArray->getYMax();

  std::vector<float> data   = dataArray->getData();
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

  double scale = ((double)m_positiveColorTable->size()-1)/zc_max;
  double ct_scale = ((double)m_positiveColorTable->size()-1);
  if ( m_intensityTable != 0 )
  {
    size_t lut_size = m_intensityTable->size();
    scale    = ((double)lut_size-1.0) / zc_max;
    ct_scale = ((double)m_positiveColorTable->size()-1);
  }

  size_t color_index;
  size_t lut_index;
  size_t image_index = 0;

  unsigned int* rgb_buffer = new unsigned int[n_rows * n_cols];
  double val = 0;
  for ( int row = (int)n_rows-1; row >= 0; row-- )
  {
    size_t data_index = row * n_cols;
    if (m_intensityTable == 0 )              // use color tables directly
    {
      for ( int col = 0; col < (int)n_cols; col++ )
      {
        val = data[data_index] * scale;
        if ( val >= 0 )                     // use positive color table
        {
          color_index = (uint)val;
          rgb_buffer[image_index] = (*m_positiveColorTable)[ color_index ];
        }
        else                               // use negative color table
        {
          color_index = (uint)(-val);
          rgb_buffer[image_index] = (*m_negativeColorTable)[ color_index ];
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
          color_index = (uint)((*m_intensityTable)[lut_index] * ct_scale );
          rgb_buffer[image_index] = (*m_positiveColorTable)[ color_index ];
        }
        else
        {
          lut_index   = (uint)(-val);
          color_index = (uint)((*m_intensityTable)[lut_index] * ct_scale );
          rgb_buffer[image_index] = (*m_negativeColorTable)[ color_index ];
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


} // namespace SpectrumView
} // namespace MantidQt
