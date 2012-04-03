
#include <iostream>

#include "MantidQtImageViewer/ImagePlotItem.h"

namespace MantidQt
{
namespace ImageView
{
  

void ImagePlotItem::SetData( DataArray*         data_array, 
                             std::vector<QRgb>* color_table )
{
  this->data_array  = data_array;
  this->color_table = color_table;
}


void ImagePlotItem::draw(       QPainter    * painter,
                          const QwtScaleMap & xMap, 
                          const QwtScaleMap & yMap,
                          const QRect       &       ) const
{
//std::cout << "ImagePlotItem::draw called =====================" << std::endl;

  if ( data_array )
  {
/*
    std::cout << "canvasRect width  = " << canvasRect.width()  << std::endl;
    std::cout << "canvasRect height = " << canvasRect.height() << std::endl;
*/
    size_t n_rows = data_array->GetNRows();
    size_t n_cols = data_array->GetNCols();
    double min    = data_array->GetDataMin();
    double max    = data_array->GetDataMax();
    double x_min  = data_array->GetXMin();
    double x_max  = data_array->GetXMax();
    double y_min  = data_array->GetYMin();
    double y_max  = data_array->GetYMax();
    float *data   = data_array->GetData();
/*
    std::cout << "x_min = " << x_min << std::endl;
    std::cout << "x_max = " << x_max << std::endl;
    std::cout << "y_min = " << y_min << std::endl;
    std::cout << "y_max = " << y_max << std::endl;
*/
    int pix_x_min = (int)xMap.transform( x_min );
    int pix_x_max = (int)xMap.transform( x_max );
    int pix_y_min = (int)yMap.transform( y_min );
    int pix_y_max = (int)yMap.transform( y_max );
/*
    std::cout << "pix_x_min = " << pix_x_min << std::endl;
    std::cout << "pix_x_max = " << pix_x_max << std::endl;
    std::cout << "pix_y_min = " << pix_y_min << std::endl;
    std::cout << "pix_y_max = " << pix_y_max << std::endl;

    std::cout << "Data Array n_rows = " << n_rows << std::endl;
*/
    double scale = 255.0/(max-min);
    double shift = -min * scale;
    size_t color_index;
    size_t image_index = 0;
    size_t data_index;
    unsigned int *rgb_data = new unsigned int[n_rows * n_cols];
    for ( int row = (int)n_rows-1; row >= 0; row-- )
    {
      data_index = row * n_cols;
      for ( int col = 0; col < (int)n_cols; col++ )
      {
        color_index = (uint)(data[data_index] * scale + shift);
        rgb_data[image_index] = (*color_table)[ color_index ];
        image_index++;
        data_index++;
      }
    }

    uchar *buffer = (uchar*)rgb_data;
    QImage image( buffer, (int)n_cols, (int)n_rows, QImage::Format_RGB32 );
    QPixmap pixmap = QPixmap::fromImage(image);
    delete[] rgb_data;                         // we can delete this, once the
                                               // pixmap is created !?

    int width  = pix_x_max - pix_x_min + 1;
    int height = pix_y_min - pix_y_max + 1;    // y-axis is inverted for image

    QPixmap scaled_pixmap = pixmap.scaled( width, height, 
                                           Qt::IgnoreAspectRatio,
                                           Qt::FastTransformation);

    painter->drawPixmap( pix_x_min, pix_y_max, scaled_pixmap );
  }
}


} // namespace MantidQt 
} // namespace ImageView 
