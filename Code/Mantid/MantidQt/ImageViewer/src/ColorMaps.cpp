
#include <iostream>
#include <sstream>

#include "MantidQtImageViewer/ColorMaps.h"

namespace MantidQt
{
namespace ImageView
{


void ColorMaps::getColorMap( ColorScale          name, 
                             int                 n_colors,
                             std::vector<QRgb> & color_table )
{
  if ( name == HEAT )
  {
    double base_red[]    = { 40, 127, 230, 255, 255 };
    double base_green[]  = { 20,   0, 127, 180, 255 };
    double base_blue[]   = { 20,   0,   0,  77, 255 };
    int    n_base_colors = 5;
    InterpolateColorScale( base_red, base_green, base_blue, 
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == GRAY )
  {
    double base_red[]   = { 30 , 255 };
    double base_green[] = { 30 , 255 };
    double base_blue[]  = { 30 , 255 };
    int    n_base_colors = 2;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == NEGATIVE_GRAY )
  {
    double base_red[]   = { 255, 30 };
    double base_green[] = { 255, 30 };
    double base_blue[]  = { 255, 30 };
    int    n_base_colors = 2;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == GREEN_YELLOW )
  {
    double base_red[]   = { 40, 255 };
    double base_green[] = { 80, 255 };
    double base_blue[]  = {  0,   0 };
    int    n_base_colors = 2;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == RAINBOW )
  {
    double base_red[]   = {  0,   0,   0, 153, 255, 255, 255 };
    double base_green[] = {  0,   0, 255, 255, 255, 153,   0 };
    double base_blue[]  = { 77, 204, 255,  77,   0,   0,   0 };
    int    n_base_colors = 7;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == OPTIMAL )
  {
    double base_red[]   = { 30, 200, 230,  30, 255 };
    double base_green[] = { 30,  30, 230,  30, 255 };
    double base_blue[]  = { 30,  30,  30, 255, 255 };
    int    n_base_colors = 5;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == MULTI )
  {
    double base_red[]   = { 30,  30,  30, 230, 245, 255 };
    double base_green[] = { 30,  30, 200,  30, 245, 255 };
    double base_blue[]  = { 30, 200,  30,  30,  30, 255 };
    int    n_base_colors = 6;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == SPECTRUM )
  {
    double base_red[]   = { 100, 235,   0, 130 };
    double base_green[] = {   0, 255, 235,   0 };
    double base_blue[]  = {   0,   0, 255, 130 };
    int    n_base_colors = 4;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
}


  /** 
   *  Build a color table by interpolating between a base set of colors.
   *  The "base" color arrays must all be of the same length ( the length
   *  being the number of base colors given.  The base color values must
   *  be between 0 and 255.  The arrays of base colors must be of length
   *  two or more.
   *  The calling routine must provide red, green and blue arrays, each 
   *  of the same length (n_colors) to hold the color table being 
   *  constructed.  
   *
   *  @param base_red       Red components of the base colors to interpolate.
   *  @param base_green     Green components of the base colors to interpolate.
   *  @param base_blue      Blue components of the base colors to interpolate.
   *  @param n_base_colors  The number of key colors that will be interpolated
   *                        form the color table. 
   *  @param n_colors       The number of colors to be created in the output
   *                        color table.
   *  @param color_table    Vector containing n_colors qRgb colors, 
   *                        interpolated from the specified base colors. 
   */

void ColorMaps::InterpolateColorScale( double base_red[],
                                       double base_green[],
                                       double base_blue[],
                                       int    n_base_colors,
                                       int    n_colors,
                                       std::vector<QRgb> & color_table )
{
  color_table.clear();
  color_table.resize( n_colors );
                                      // first output color is first base color
  color_table[0] = qRgb( (unsigned char)base_red[0], 
                         (unsigned char)base_green[0], 
                         (unsigned char)base_blue[0]  );

                                      // last output color is last base color
  int last_out = n_colors - 1;         
  int last_in  = n_base_colors - 1;
  color_table[last_out] = qRgb( (unsigned char)base_red[last_in],       
                                (unsigned char)base_green[last_in],      
                                (unsigned char)base_blue[last_in]  );

                                       // interpolate remaining output colors
  for ( int i = 1; i < last_out; i++ )
  {
                                      // fraction of way along output indices
    double t_out = (double)i / (double)last_out;

    double float_index = t_out * last_in; // corresponding "floating point"
                                          // index in array of input colors
    int base_index = (int)float_index;

    double t = float_index - (double)base_index;

    color_table[i] = qRgb(
                     (unsigned char) ( (1.0-t) * base_red[base_index]+
                                          t    * base_red[base_index + 1] ),
                     (unsigned char) ( (1.0-t) * base_green[base_index]+
                                          t    * base_green[base_index + 1] ),
                     (unsigned char) ( (1.0-t) * base_blue[base_index]+
                                          t    * base_blue[base_index + 1] ) );
  }
}



} // namespace MantidQt 
} // namespace ImageView 
