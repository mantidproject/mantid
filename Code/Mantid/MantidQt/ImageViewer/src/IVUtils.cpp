#include <iostream>
#include <sstream>
#include <math.h>

#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{

/**
 * Extract a double from the specified string, if possible.
 * 
 * @param text   The string containing a double value
 * @param value  Set to the value from the string if the conversion
 *               worked.
 * @return true if the string could be successfully converted to a double,
 *         and return false otherwise.
 */
bool IVUtils::StringToDouble( std::string  text,
                              double      &value )
{
  std::istringstream strs(text);
  if ( strs >> value )
  {
    return true;
  }
  return false;
}


/**
 *  Get a formatted string form of the specified double precision value.
 *
 *  @param width     The total number of characters to be used in the 
 *                   formatted string
 *  @param precison  The number of significant figures to use 
 *  @param value     The double precsion number to be formatted
 *  @param str       String that will be set to the formatted number
 */
void IVUtils::Format( int            width,
                      int            precision,
                      double         value,
                      std::string  & str )
{
  std::ostringstream strs;
  strs.setf(std::ios::fixed);
  strs.width( width );
  strs.precision( precision );
  strs.setf(std::ostringstream::showpoint);

  strs << value;
  str = strs.str();
}


/**
 * Find a non-degenerate interval containing all the specified values.
 * If there are more than one values in the list, min will be set to 
 * the smallest value and max will be set to the largest value.  If there
 * is only on value in the list, min will be set to 90% of that value and
 * max will be set to 110% of that value.  If the only value in the list
 * is zero, min will be set to -1 and max will be set to 1.  In any case
 * the interval [min,max] will contain all values in the list and min will
 * be strictly less than max.
 *
 * @param values  List of values to be bounded by min and max.
 * @param min     Set to be less than or equal to all values in the list
 *                and strictly less than max.
 * @param max     Set to be greater than or equal to all values in the list
 *                and strictly more than min.
 */
void IVUtils::FindValidInterval( const QVector<double>  & values,
                                       double           & min,
                                       double           & max )
{
  double val;
  min = values[0];
  max = min;
  for ( int i = 1; i < (int)values.size(); i++ )
  {
    val = values[i];
    if ( min > val )
      min = val;
    if ( max < val )
      max = val;
  }
  FindValidInterval( min, max );
}


/**
 * Adjust min and max so that min is strictly less than max.  If min > max
 * the values are swapped.  If min = max != 0, they will be shifted off from 
 * their initial common value by 10%.  If min = max = 0, they will be set
 * to -1 and 1, respectively.
 *
 * @param min     Set to be strictly less than max. 
 * @param max     Set to be strictly greater than min.
 */
void IVUtils::FindValidInterval( double  & min,
                                 double  & max )
{

  if ( max == min )           // adjust values so they are not equal
  {
    if ( min == 0 )
    {
      min = -1,
      max =  1;
    }
    else
    {
      max = 1.1 * max;
      min = 0.9 * min;
    }
  }

  if ( min > max )            // fix the order
  {
    double temp = min;
    min = max;
    max = temp;
  }
}


/**
 * Calculate a point in [new_min,new_max] by linear interpolation, and
 * clamp the result to be in the interval [new_min,new_max].
 * @param min       Left endpoint of original interval
 * @param max       Right endpoint of original interval
 * @param val       Reference point in orignal interval
 * @param new_min   Left endpoint of new interval
 * @param new_max   Right endpoint of new interval
 * @param new_val   Point in new interval that is placed in [new_min,new_max]
 *                  in the same proportion as val is in [min,max].  The 
 *                  resulting new_val will be clamped to be in the new
 *                  interval, even if val is outside of the original interval.
 */
bool IVUtils::Interpolate( double   min,
                           double   max,
                           double   val,
                           double   new_min,
                           double   new_max,
                           double & new_val )
{
  new_val = (val - min)/( max - min ) * (new_max - new_min) + new_min;
  if ( new_val < new_min || new_val > new_max )
    return false;
  else
    return true;
}


/**
 *  Find a new interval [min,max] with boundaries aligned with the underlying
 *  data bin boundaries, then set first_index to the index of the bin, 
 *  corresponding to the min value and set the number of steps to the smaller
 *  of the number of steps in the data, and the initial value of the number
 *  of steps.
 *
 *  @param global_min   Smallest value covered by the underlying data
 *  @param global_max   Largest value covered by the underlying data
 *  @param global_steps Number of uniform bins the underlying data is 
 *                      divided into on the interval [global_min,global_max].
 *  @param first_index  This will be set to the bin number containing the
 *                      specified min value.
 *  @param min          On input this should be smallest value of interest 
 *                      in the interval.  This will be adjusted to be the 
 *                      left bin boundary of the bin containing the specified
 *                      min value.
 *  @param max          On input this should be largest value of interest 
 *                      in the interval.  This will be adjusted to be the 
 *                      right bin boundary of the bin containing the specified
 *                      max value, if max is in the interior of a bin.
 *  @param steps        On input this should be the number of bins desired
 *                      between the min and max values.  This will be adjusted
 *                      to be more than the number os steps available.
 */
bool IVUtils::CalculateInterval( double   global_min,
                                 double   global_max,
                                 size_t   global_steps,
                                 size_t & first_index,
                                 double & min,
                                 double & max,
                                 size_t & steps )
{
  double d_index;
                                         // find bin containing min.......
  Interpolate( global_min, global_max,   min,
                      0.0, (double)global_steps, d_index );

  int min_index = (int)floor(d_index);   // min_index is the number of the bin
                                         // containing min
  if ( min_index < 0 )
    min_index = 0;
                                         // now set min to the value at the 
                                         // left edge of bin at min_index
  Interpolate( 0.0,        (double)global_steps, (double)min_index,
               global_min, global_max,   min );

                                         // find bin containing max........ 
  Interpolate( global_min, global_max,   max,
                      0.0, (double)global_steps, d_index );

  int max_index = (int)ceil(d_index) - 1;// max index is the number of the bin
                                         // containing max, or with max as
                                         // right hand endpoint
  if ( max_index >= (int)global_steps )
    max_index = max_index - 1;

                                         // now set max to the value at the
                                         // right edge of bin max_index
  Interpolate( 0,          (double)global_steps, (double)(max_index + 1),
               global_min, global_max,   max );

  first_index = min_index;

  size_t source_steps = max_index - min_index + 1;
  if ( steps > source_steps )
    steps = source_steps;

  return true;
}

/*
int main()
{
  double new_val;
  IVUtils::Interpolate( -10, 20, 9, -100, 200, new_val );

  std::cout << "new_val = " << new_val << std::endl;

  size_t first_index;
  double min = 150;
  double max = 160;
  size_t steps = 10;

  IVUtils::CalculateInterval(100, 200, 100, 
                             first_index, min, max, steps);

  std::cout << "first_index = " << first_index << std::endl;
  std::cout << "min         = " << min << std::endl;
  std::cout << "max         = " << max << std::endl;
  std::cout << "steps       = " << steps << std::endl;

  return 0;
}
*/


} // namespace MantidQt 
} // namespace ImageView 
