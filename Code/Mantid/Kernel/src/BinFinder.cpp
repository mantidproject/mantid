#include "MantidKernel/BinFinder.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Kernel
{

  /** Constructor. Sets up the calculation for later.
   *
   * @param binParams: the binning parameters, as a vector of doubles. E.g.
   *    0, 1.0, 100, -0.5, 1e7:
   *    bins in step to 1.0 from 0 to 100; then log steps up to 1e7
   */
  BinFinder::BinFinder(const std::vector<double>& binParams)
  {
    boundaries.clear();
    stepSizes.clear();

    int n = binParams.size();
    if (n < 3)
      throw std::invalid_argument("BinFinder: not enough bin parameters.");
    if (n % 2 == 0)
      throw std::invalid_argument("BinFinder: the number of bin parameters should be odd.");

    for (int i=0; i<n/2; i++)
    {
      //The boundaries
      double min = binParams[i*2];
      double max = binParams[i*2+2];
      boundaries.push_back( min );
      boundaries.push_back( max );
      //The step
      double step = binParams[i*2+1];
      stepSizes.push_back( step );
      if (step == 0)
        throw std::invalid_argument("BinFinder: step size of 0.");
      if ((step < 0) && (min <= 0))
        throw std::invalid_argument("BinFinder: logarithmic binning with 0.0 starting bin.");
      if (max <= min)
        throw std::invalid_argument("BinFinder: final bin must be > starting bin boundary.");

      //Pre-do some calculations for log binning.
      if (step < 0)
      {
        logSteps.push_back( log(1.0 + abs(step)) );
        logBoundaries.push_back( log(min) );
        logBoundaries.push_back( log(max) );
      }
      else
      {
        //Empty log values; these won't be used
        logSteps.push_back( 0 );
        logBoundaries.push_back( 0 );
        logBoundaries.push_back( 0 );
      }
    }
    //How many binning regions?
    numRegions = stepSizes.size();
  }

  /// Destructor
  BinFinder::~BinFinder()
  {

  }

  /** Find the bin index for a value.
   * @param x: x-value to histogram
   * @return an int corresponding to the bin index to use, or -1 if out of bounds.
   */
  int BinFinder::bin(double x)
  {
    int index;
    double min, max;

    //Too small?
    if (x < boundaries[0])
      return -1;

    //Find which binning region to use
    int i=-1;
    for (i=0; i < numRegions; i++)
    {
      min = boundaries[i];
      max = boundaries[i+1];
      if ( (x >= min) && (x < max) )
        break;
    }
    //Didn't find it?
    if (i >= numRegions)
      return -1;

    //Step size in this region
    double step = stepSizes[i];
    if (step > 0)
    {
      //Linear binning. Truncate when you divide by the step size
      index = (x - min) / step;
      return index;
    }
    else
    {
      /** Log binning formula for bin index n:
       *  x_n = min * ( 1 + |step| ) ^ n
       *  log(x_n) = log(min) + n * log(1+|step|)
       *  therefore
       *  n = (log(x_n) - log(min))/log(1+|step|)
       */

      double log_x = log(x); //Just one log to call per event!
      double log_step = logSteps[i];
      double log_min = logBoundaries[i];
      index = (log_x - log_min) / log_step;
      return index;
    }



  }


} } //Namespace
