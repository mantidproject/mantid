
#include <iostream>
#include <sstream>
#include <math.h>

#include "MantidQtSpectrumViewer/ColorMaps.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Get a color map of the specified type, with the specified number of
 * colors by interpolating between key colors.
 * @param name         The name of the color scale as listed in the
 *                     enum ColorMaps::ColorScale
 * @param n_colors     The number of colors to use when forming the
 *                     color map.  The number of colors must be at least 7
 *                     for some of the constructed color maps.
 * @param color_table  Vector of colors that will be cleard and filled out
 *                     with the requested color map.
 */
void ColorMaps::GetColorMap( ColorScale          name,
                             size_t              n_colors,
                             std::vector<QRgb> & color_table )
{
  if ( name == HEAT )
  {
    double base_red[]    = { 40, 127, 230, 255, 255 };
    double base_green[]  = { 20,   0, 127, 180, 255 };
    double base_blue[]   = { 20,   0,   0,  77, 255 };
    size_t n_base_colors = 5;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == GRAY )
  {
    double base_red[]   = { 30 , 255 };
    double base_green[] = { 30 , 255 };
    double base_blue[]  = { 30 , 255 };
    size_t n_base_colors = 2;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == NEGATIVE_GRAY )
  {
    double base_red[]   = { 255, 30 };
    double base_green[] = { 255, 30 };
    double base_blue[]  = { 255, 30 };
    size_t n_base_colors = 2;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == GREEN_YELLOW )
  {
    double base_red[]   = { 40, 255 };
    double base_green[] = { 80, 255 };
    double base_blue[]  = {  0,   0 };
    size_t n_base_colors = 2;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == RAINBOW )
  {
    double base_red[]   = {  0,   0,   0, 153, 255, 255, 255 };
    double base_green[] = {  0,   0, 255, 255, 255, 153,   0 };
    double base_blue[]  = { 77, 204, 255,  77,   0,   0,   0 };
    size_t n_base_colors = 7;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == OPTIMAL )
  {
    double base_red[]   = { 30, 200, 230,  30, 255 };
    double base_green[] = { 30,  30, 230,  30, 255 };
    double base_blue[]  = { 30,  30,  30, 255, 255 };
    size_t n_base_colors = 5;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == MULTI )
  {
    double base_red[]   = { 30,  30,  30, 230, 245, 255 };
    double base_green[] = { 30,  30, 200,  30, 245, 255 };
    double base_blue[]  = { 30, 200,  30,  30,  30, 255 };
    size_t n_base_colors = 6;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
  else if ( name == SPECTRUM )
  {
    double base_red[]   = { 100, 235,   0, 130 };
    double base_green[] = {   0, 255, 235,   0 };
    double base_blue[]  = {   0,   0, 255, 130 };
    size_t n_base_colors = 4;
    InterpolateColorScale( base_red, base_green, base_blue,
                           n_base_colors, n_colors, color_table );
  }
}


/**
 * Get an intensity lookup table to adjust the apparent brightness of a
 * displayed image.  The lookup table makes an adjustment to the image
 * intensity similar to a gamma correction, but over a wide range.  The
 * table will be created with the specified number of entries and the
 * entries will increase monotonically (but non-linearly) from 0 to 1.
 *
 * @param control_s        Control parameter between 0 and 100.  When
 *                         the parameter is at 0, the look up table is
 *                         linear.  As the parameter increases, low
 *                         intensity values will increasingly get larger
 *                         scale factors.
 * @param n_entries        The number of entries to create in the table.
 *                         This controls the resolution of the mapping and
 *                         should be quite large (10,000-100,000) to preserve
 *                         smooth color transitions even a lower intensity
 *                         values, when the control parameter is large.
 * @param intensity_table
 */
void ColorMaps::GetIntensityMap( double                control_s,
                                 size_t                n_entries,
                                 std::vector<double> & intensity_table )
{

  intensity_table.clear();
  intensity_table.resize( n_entries );
                                        // restrict control range to [0,100]
  double MAX_CONTROL = 100.0;
  if ( control_s > MAX_CONTROL )
  {
    control_s = MAX_CONTROL;
  }
  else if ( control_s < 0.0 )
  {
    control_s = 0.0;
  }

  if ( control_s == 0.0 )                // just use linear scale, 0 -> 1
  {
    for ( size_t i = 0; i < n_entries; i++ )
    {
      intensity_table[i] = (double)i / (double)(n_entries - 1);
    }
  }
  else                                   // build log-shaped correction scale
  {
                                         // first map control value
                                         // exponentially to make the control
                                         // parameter act more linearly
    double s     = exp( 20.0 * control_s / MAX_CONTROL ) + 0.1;
    double scale = 1.0 / log( s );
    for ( size_t i = 0; i < n_entries - 1; i++ )
    {
      intensity_table[i] = scale *
                       log( 1.0+((s-1.0)*(double)i) / (double)(n_entries-1) );
    }
    intensity_table[n_entries - 1] = 1.0;  // this could have been calculated
                                           // by running the loop one step
                                           // further, but due to rounding
                                           // errors, it might exceed 1.
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
                                       size_t n_base_colors,
                                       size_t n_colors,
                                       std::vector<QRgb> & color_table )
{
  color_table.clear();
  color_table.resize( n_colors );
                                      // first output color is first base color
  color_table[0] = qRgb( (unsigned char)base_red[0],
                         (unsigned char)base_green[0],
                         (unsigned char)base_blue[0]  );

                                      // last output color is last base color
  size_t last_out = n_colors - 1;
  size_t last_in  = n_base_colors - 1;
  color_table[last_out] = qRgb( (unsigned char)base_red[last_in],
                                (unsigned char)base_green[last_in],
                                (unsigned char)base_blue[last_in]  );

                                       // interpolate remaining output colors
  for ( size_t i = 1; i < last_out; i++ )
  {
                                      // fraction of way along output indices
    double t_out = (double)i / (double)last_out;

    double float_index = t_out * (double)last_in;
                                          // corresponding "floating point"
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

} // namespace SpectrumView
} // namespace MantidQt
