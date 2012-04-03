#include <iostream>
#include <sstream>
#include <math.h>

#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{


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
  if ( max == min )
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
    if ( min > max )    // NOTE: They could be < 0.
    {
      double temp = min;
      min = max;
      max = temp;
    }
  }
}


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
 *  Find a new interval [min,max] with values aligned with the underlying
 *  data bin boundaries, and set first_index to the index of the bin, 
 *  corresponding to the min value and set the number of steps to the smaller
 *  of the number of steps in the data, and the initial value of the number
 *  of steps.
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
  Interpolate( global_min, global_max,   min,
                      0.0, (double)global_steps, d_index );

  int min_index = (int)d_index;        // min is coordinate at the left edge of
                                       // bin at min_index
  if ( min_index < 0 )
    min_index = 0;

  Interpolate( 0.0,        (double)global_steps, (double)min_index,
               global_min, global_max,   min );

  Interpolate( global_min, global_max,   max,
                      0.0, (double)global_steps, d_index );

  int max_index = (int)ceil(d_index) - 1;// max is coordinate at the right edge 
                                         // of bin at max_index.
  if ( max_index >= (int)global_steps )
    max_index = max_index - 1;

  Interpolate( 0,          (double)global_steps, (double)(max_index + 1),
               global_min, global_max,   max );

  first_index = (int)min_index;

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
