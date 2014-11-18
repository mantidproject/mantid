#include <iostream>
#include <sstream>
#include <math.h>

#include "MantidQtSpectrumViewer/SVUtils.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Get a formatted string form of the specified double precision value.
 *
 *  @param width     The total number of characters to be used in the
 *                   formatted string
 *  @param precision  The number of significant figures to use
 *  @param value     The double precsion number to be formatted
 *  @param str       String that will be set to the formatted number
 */
void SVUtils::Format( int            width,
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
 *  Push a name, value pair onto a vector of strings.  The value is converted
 *  to a string using the specified width and precision.
 *
 *  @param name      String name that is first pushed on the list
 *  @param width     The total number of characters to be used when formatting
 *                   the value
 *  @param precision The number of significant figures to use
 *  @param value     The double precsion number to be formatted and pushed on
 *                   the list
 *  @param list      The list of strings to which the name,value pair is added.
 */
void SVUtils::PushNameValue( const std::string               & name,
                                   int                         width,
                                   int                         precision,
                                   double                      value,
                                   std::vector<std::string>  & list )
{
  list.push_back( name );

  std::string value_str;
  Format( width, precision, value, value_str );
  list.push_back( value_str );
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
bool SVUtils::FindValidInterval( const QVector<double>  & values,
                                       double           & min,
                                       double           & max )
{
  min = values[0];
  max = min;
  for ( int i = 1; i < (int)values.size(); i++ )
  {
    double val = values[i];
    if ( min > val )
      min = val;
    if ( max < val )
      max = val;
  }
  return FindValidInterval( min, max );
}


/**
 * Adjust min and max so that min is strictly less than max.  If min > max
 * the values are swapped.  If min = max != 0, they will be shifted off from
 * their initial common value by 10%.  If min = max = 0, they will be set
 * to -1 and 1, respectively.
 *
 * @param min     Set to be strictly less than max.
 * @param max     Set to be strictly greater than min.
 *
 * @return true if the original values were OK and are unchanged, return
 *         false if min or max was altered to make a valid interval.
 */
bool SVUtils::FindValidInterval( double  & min,
                                 double  & max )
{
  bool valuesOK = true;

  if ( max == min )  // adjust values so they are not equal
  {
    valuesOK = false;
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

  if ( min > max )  // fix the order
  {
    valuesOK = false;
    double temp = min;
    min = max;
    max = temp;
  }

  return valuesOK;
}


/**
 * Adjust min and max so that min is strictly less than max, and both are
 * greater than 0.  If min > max the values are swapped.  If min = max > 0,
 * they will be shifted off from their initial common value by factors of 10.
 * If min = max = 0, they will be set to 0.1 and 10, respectively.
 *
 * @param min     Set to be strictly less than max and more than 0.
 * @param max     Set to be strictly greater than min.
 *
 * @return true if the original values were OK and are unchanged, return
 *         false if min or max was altered to make a valid interval.
 */
bool SVUtils::FindValidLogInterval( double  & min,
                                    double  & max )
{
  bool valuesOK = true;
  if ( min < 0 )
  {
    std::cout << "min < 0 " << min << std::endl;
    valuesOK = false;
    min = -min;
  }

  if ( max < 0 )
  {
    std::cout << "max < 0 " << max << std::endl;
    valuesOK = false;
    max = -max;
  }

  if ( min > max )  // Fix the order
  {
    std::cout << "min > max " << min << " > " << max << std::endl;
    valuesOK = false;
    double temp = min;
    min = max;
    max = temp;
  }

  // Raise min, so the interval covers 2 orders of magnitude
  if ( min == 0 && max > 0 )
  {
    std::cout << "min == 0, max > 0 " << min << ", " << max << std::endl;
    valuesOK = false;
    min = 0.01 * max;
  }
  else if ( max == min )  // Adjust values so they are not equal
  {
    valuesOK = false;
    std::cout << "min == max " << min << " == " << max << std::endl;
    if ( min == 0 )
    {
      min = 0.1,
      max =  10;
    }
    else
    {
      max =  10 * max;
      min = 0.1 * min;
    }
  }

  return valuesOK;
}


/**
 *  Calculate the number of steps required to go from min to max on either
 *  a linear or logarithmic scale.
 *  @param min   Lowest value on scale, must be positive for log scale, and
 *               must always be less than max.
 *  @param max   Highest value on scale, must be positive for log scale, and
 *               must always be more than min.
 *  @param step  Must be more than zero for linear scale and less than zero
 *               for log scale.  This must NOT be zero and should be less
 *               than max - min in absolute value.
 *  @return the number of bins from min to max, if the interval is divided
 *          linearly or "lograrithmically".  If the data is invalid, this
 *          will return 0.
 */
int SVUtils::NumSteps( double min, double max, double step )
{
  int numBins = 0;

  if ( step == 0 || (max-min) <= 0 || (step < 0 && min <= 0) )
  {
    return 0;
  }

  if ( step > 0 )                          // uniform steps
  {
    numBins = (int)(( max - min ) / step);
  }
  else if ( step < 0 )                     // log steps
  {

  /* Interpret step as the negative of the fractional increase in the */
  /* first bin boundary, relative to the zeroth bin boundary (min). */
  /* This is the convention followed by the Rebin() algorithm in Mantid. */

    numBins = (int)ceil( (log(max) - log(min))/log(1 - step) );
    if ( numBins < 1 )
      numBins = 1;

  /* This formula assumes a negative step indicates a log scale with */
  /* the size of the first bin specified by |step|.  This is not the */
  /* convention used in the Rebin algorithm, so we have commented */
  /* this out and use the Mantid convention. */

  //numBins = (int)ceil( (log(max) - log(min))/log(1 - step/min) );
  }

  return numBins;
}


/**
 * Calculate a point in [newMin,newMax] by linear interpolation, and
 * clamp the result to be in the interval [newMin,newMax].
 * @param min       Left endpoint of original interval
 * @param max       Right endpoint of original interval
 * @param val       Reference point in orignal interval
 * @param newMin   Left endpoint of new interval
 * @param newMax   Right endpoint of new interval
 * @param newVal   Point in new interval that is placed in [newMin,newMax]
 *                  in the same proportion as val is in [min,max].
 * @return true if the calculated value is in [newMin,newMax] and false
 *         if it is outside of the interval.
 */
bool SVUtils::Interpolate( double   min,
                           double   max,
                           double   val,
                           double   newMin,
                           double   newMax,
                           double & newVal )
{
  newVal = (val - min)/( max - min ) * (newMax - newMin) + newMin;

  if ( newVal < newMin || newVal > newMax )
    return false;
  else
    return true;
}


/**
 * Calculate the value in [newMin,newMax] on a logarithmic scale that
 * would correspond to the point val on a linear scale on [min,max].
 * For example, if val was half way from min to max, and the log scale
 * extended from newMin = 1 to newMax = 100, then newVal would return 10,
 * since 10 is half way along a log scale from 1 to 100.
 * Clamp the result to be in the interval [newMin,newMax].
 * @param min       Left endpoint of original interval with linear scale
 * @param max       Right endpoint of original interval with linear scale
 * @param val       Reference point in orignal interval
 * @param newMin   Left endpoint of new interval with log scale
 * @param newMax   Right endpoint of new interval with log scale
 * @param newVal   Point in new interval that is placed in [newMin,newMax]
 *                  in the same proportion as val is in [min,max].
 * @return true if the calculated value is in [newMin,newMax] and false
 *         if it is outside of the interval.
 */
bool SVUtils::LogInterpolate( double   min,
                              double   max,
                              double   val,
                              double   newMin,
                              double   newMax,
                              double & newVal )
{
  newVal = newMin * exp( (val-min)/(max-min) * log ( newMax/newMin ));

  if ( newVal < newMin || newVal > newMax )
    return false;
  else
    return true;
}


/**
 *  Find a new interval [min,max] with boundaries aligned with the underlying
 *  data bin boundaries, then set firstIndex to the index of the bin,
 *  corresponding to the min value and set the number of steps to the smaller
 *  of the number of steps in the data, and the initial value of the number
 *  of steps.  NOTE: This calculation is needed for displaying a fixed array
 *  of data that should not be rebinned.
 *
 *  @param globalMin    Smallest value covered by the underlying data
 *  @param globalMax    Largest value covered by the underlying data
 *  @param globalSteps  Number of uniform bins the underlying data is
 *                      divided into on the interval [globalMin,globalMax].
 *  @param firstIndex   This will be set to the bin number containing the
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
 *                      to be no more than the number of steps available.
 */
bool SVUtils::CalculateInterval( double   globalMin,
                                 double   globalMax,
                                 size_t   globalSteps,
                                 size_t & firstIndex,
                                 double & min,
                                 double & max,
                                 size_t & steps )
{
  double index;

  // Find bin containing min
  Interpolate( globalMin, globalMax,           min,
               0.0,       (double)globalSteps, index );

  // min_index is the number of the bin containing min
  int min_index = (int)floor(index);

  if ( min_index < 0 )
    min_index = 0;

  // Now set min to the value at the left edge of bin at min_index
  Interpolate( 0.0,       (double)globalSteps, (double)min_index,
               globalMin, globalMax,           min );

  // Find bin containing max
  Interpolate( globalMin, globalMax,           max,
               0.0,       (double)globalSteps, index );

  /* max_index is the number of the bin */
  /* containing max, or with max as */
  /* right hand endpoint */
  int max_index = (int)ceil(index) - 1;

  if ( max_index >= (int)globalSteps )
    max_index = (int)globalSteps - 1;

  // Now set max to the value at the right edge of bin max_index
  Interpolate( 0,         (double)globalSteps, (double)(max_index + 1),
               globalMin, globalMax,           max );

  firstIndex = min_index;

  size_t source_steps = max_index - min_index + 1;
  if ( steps > source_steps )
    steps = source_steps;

  return true;
}

} // namespace SpectrumView
} // namespace MantidQt
