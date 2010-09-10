#include <stdexcept>
#include <cmath>

#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <vector>
#include <numeric>
#include <limits>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>

namespace Mantid
{
namespace Kernel
{
namespace VectorHelper
{

/** Creates a new output X array given a 'standard' set of rebinning parameters.
 *  @param[in]  params Rebin parameters input [x_1, delta_1,x_2, ... ,x_n-1,delta_n-1,x_n]
 *  @param[out] xnew   The newly created axis resulting from the input params
 *  @return The number of bin boundaries in the new axis
 **/
int DLLExport createAxisFromRebinParams(const std::vector<double>& params, std::vector<double>& xnew)
{
  double xs;
  int ibound(2), istep(1), inew(1);
  int ibounds = params.size(); //highest index in params array containing a bin boundary
  int isteps = ibounds - 1; // highest index in params array containing a step
  xnew.clear();

  double xcurr = params[0];
  xnew.push_back(xcurr);

  while ((ibound <= ibounds) && (istep <= isteps))
  {
    // if step is negative then it is logarithmic step
    if (params[istep] >= 0.0)
      xs = params[istep];
    else
      xs = xcurr * fabs(params[istep]);

    if (fabs(xs) == 0.0)
    {
      //Someone gave a 0-sized step! What a dope.
      throw std::runtime_error("Invalid binning step provided! Can't creating binning axis.");
    }

    /* continue stepping unless we get to almost where we want to */
    // Ensure that last bin in a range is not smaller than 25% of previous bin
    if ( (xcurr + xs*1.25) <= params[ibound] )
    {
      xcurr += xs;
    }
    else
    {
      xcurr = params[ibound];
      ibound += 2;
      istep += 2;
    }
    xnew.push_back(xcurr);
    inew++;

//    if (xnew.size() > 10000000)
//    {
//      //Max out at 1 million bins
//      throw std::runtime_error("Over ten million binning steps created. Exiting to avoid infinite loops.");
//      return inew;
//    }
  }

  return inew;
}

/** Rebins data according to a new output X array
 *
 *  @param[in] xold Old X array of data. 
 *  @param[in] yold Old Y array of data. Must be 1 element shorter than xold.
 *  @param[in] eold Old error array of data. Must be same length as yold.
 *  @param[in] xnew X array of data to rebin to.
 *  @param[out] ynew Rebinned data. Must be 1 element shorter than xnew.
 *  @param[out] enew Rebinned errors. Must be same length as ynew.
 *  @param[in] distribution Flag defining if distribution data (true) or not (false).
 *  @param[in] addition If true, rebinned values are added to the existing ynew/enew vectors.
 *                      NOTE THAT, IN THIS CASE THE RESULTING enew WILL BE THE SQUARED ERRORS
 *                      AND THE ynew WILL NOT HAVE THE BIN WIDTH DIVISION PUT IN!
 *  @throw runtime_error Thrown if algorithm cannot execute.
 *  @throw invalid_argument Thrown if input to function is incorrect.
 **/
void rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
      const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, 
      bool distribution, bool addition)
{
  // Make sure y and e vectors are of correct sizes
  const size_t size_xold = xold.size();
  if (size_xold != (yold.size() + 1) || size_xold != (eold.size() + 1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");
  const size_t size_xnew = xnew.size();
  if (size_xnew != (ynew.size() + 1) || size_xnew != (enew.size() + 1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");

  int size_yold = yold.size();
  int size_ynew = ynew.size();

  if (!addition)
  {
    // Make sure ynew & enew contain zeroes
    ynew.assign(size_ynew, 0.0);
    enew.assign(size_ynew, 0.0);
  }

  int iold = 0, inew = 0;
  double xo_low, xo_high, xn_low, xn_high, delta(0.0), width;

  while ((inew < size_ynew) && (iold < size_yold))
  {
    xo_low = xold[iold];
    xo_high = xold[iold + 1];
    xn_low = xnew[inew];
    xn_high = xnew[inew + 1];
    if (xn_high <= xo_low)
      inew++; /* old and new bins do not overlap */
    else if (xo_high <= xn_low)
      iold++; /* old and new bins do not overlap */
    else
    {
      //        delta is the overlap of the bins on the x axis
      //delta = std::min(xo_high, xn_high) - std::max(xo_low, xn_low);
      delta = xo_high < xn_high ? xo_high : xn_high;
      delta -= xo_low > xn_low ? xo_low : xn_low;
      width = xo_high - xo_low;
      if ((delta <= 0.0) || (width <= 0.0))
      {
        // No need to throw here, just return (ynew & enew will be empty)
        //throw std::runtime_error("rebin: no bin overlap detected");
        return;
      }
      /*
       *        yoldp contains counts/unit time, ynew contains counts
       *	       enew contains counts**2
       *        ynew has been filled with zeros on creation
       */
      if (distribution)
      {
        // yold/eold data is distribution
        ynew[inew] += yold[iold] * delta;
        // this error is calculated in the same way as opengenie
        enew[inew] += eold[iold] * eold[iold] * delta * width;
      }
      else
      {
        // yold/eold data is not distribution
        // do implicit division of yold by width in summing.... avoiding the need for temporary yold array
        // this method is ~7% faster and uses less memory
        ynew[inew] += yold[iold] * delta / width; //yold=yold/width
        // eold=eold/width, so divide by width**2 compared with distribution calculation
        enew[inew] += eold[iold] * eold[iold] * delta / width;
      }
      if (xn_high > xo_high)
      {
        iold++;
      }
      else
      {
        inew++;
      }
    }
  }

  if (!addition) // If using the addition facility, have to do bin width and sqrt errors externally
  {
    if (distribution)
    {
      /*
       * convert back to counts/unit time
       */
      for (int i = 0; i < size_ynew; ++i)
      {
        {
          width = xnew[i + 1] - xnew[i];
          if (width != 0.0)
          {
            ynew[i] /= width;
            enew[i] = sqrt(enew[i]) / width;
          }
          else
          {
            throw std::invalid_argument("rebin: Invalid output X array, contains consecutive X values");
          }
        }
      }
    }
    else
    {
      // non-distribution, just square root final error value
      typedef double (*pf)(double);
      pf uf = std::sqrt;
      std::transform(enew.begin(), enew.end(), enew.begin(), uf);
    }
  }

  return; //without problems
}

//-------------------------------------------------------------------------------------------------
/** Rebins histogram data according to a new output X array. Should be faster than previous one.
 *  @author Laurent Chapon 10/03/2009
 *
 *  @param[in] xold Old X array of data. 
 *  @param[in] yold Old Y array of data. Must be 1 element shorter than xold.
 *  @param[in] eold Old error array of data. Must be same length as yold.
 *  @param[in] xnew X array of data to rebin to.
 *  @param[out] ynew Rebinned data. Must be 1 element shorter than xnew.
 *  @param[out] enew Rebinned errors. Must be same length as ynew.
 *  @param[in] addition If true, rebinned values are added to the existing ynew/enew vectors.
 *                      NOTE THAT, IN THIS CASE THE RESULTING enew WILL BE THE SQUARED ERRORS!
 *  @throw runtime_error Thrown if vector sizes are inconsistent
 **/
void rebinHistogram(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
    const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew,bool addition)
{
  // Make sure y and e vectors are of correct sizes
  const size_t size_yold = yold.size();
  if ( xold.size() != (size_yold+ 1) || size_yold != eold.size() )
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");
  const size_t size_ynew = ynew.size();
  if ( xnew.size() != (size_ynew + 1) || size_ynew != enew.size() )
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");

  // If not adding to existing vectors, make sure ynew & enew contain zeroes
  if (!addition)
  {
    ynew.assign(size_ynew, 0.0);
    enew.assign(size_ynew, 0.0);
  }

  // Find the starting points to avoid wasting time processing irrelevant bins
  size_t iold = 0, inew = 0;   // iold/inew is the bin number under consideration (counting from 1, so index+1)
  if (xnew.front() > xold.front())
  {
    std::vector<double>::const_iterator it = std::upper_bound(xold.begin(), xold.end(), xnew.front());
    if (it == xold.end()) return;
//      throw std::runtime_error("No overlap: max of X-old < min of X-new");
    iold = std::distance(xold.begin(), it) - 1; // Old bin to start at (counting from 0)
  }
  else
  {
    std::vector<double>::const_iterator it = std::upper_bound(xnew.begin(), xnew.end(), xold.front());
    if (it == xnew.end()) return;
//      throw std::runtime_error("No overlap: max of X-new < min of X-old");
    inew = std::distance(xnew.begin(), it) - 1; // New bin to start at (counting from 0)
  }
  
  double frac, fracE;
  double width, overlap;
  
  //loop over old vector from starting point calculated above
  for ( ; iold<size_yold; ++iold )
  {
    // If current old bin is fully enclosed by new bin, just unload the counts
    if ( xold[iold+1] <= xnew[inew+1] )
    {
      ynew[inew] += yold[iold];
      enew[inew] += std::pow(eold[iold], 2);
      // If the upper bin boundaries were equal, then increment inew
      if ( xold[iold+1] == xnew[inew+1] ) inew++;
    }
    else
    {
      // This is the counts per unit X in current old bin
      width = (xold[iold+1] - xold[iold]);
      frac = yold[iold] / width;
      fracE = std::pow(eold[iold], 2) / width;
    
      // Now loop over bins in new vector overlapping with current 'old' bin
      while ( inew<size_ynew && xnew[inew+1] <= xold[iold+1] )
      {
        overlap = xnew[inew+1] - std::max(xnew[inew],xold[iold]);
        ynew[inew] += frac * overlap;
        enew[inew] += fracE * overlap;
        ++inew;
      }
    
      // Stop if at end of new X range
      if (inew==size_ynew) break;
      
      // Unload the rest of the current old bin into the current new bin
      overlap = xold[iold+1] - xnew[inew];
      ynew[inew] += frac * overlap;
      enew[inew] += fracE * overlap;
    }
  } // loop over old bins

  if (!addition) //If this used to add at the same time then not necessary (should be done externally)
  {
    //Now take the root-square of the errors
    typedef double (*pf)(double);
    pf uf = std::sqrt;
    std::transform(enew.begin(), enew.end(), enew.begin(), uf);
  }

  return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Convert the given set of bin boundaries into bin centre values
 * @param bin_edges A vector of values specifying bin boundaries
 * @param bin_centres An output vector of bin centre values.
*/
void convertToBinCentre(const std::vector<double> & bin_edges, std::vector<double> & bin_centres)
{
  const std::vector<double>::size_type npoints = bin_edges.size();
  if( bin_centres.size() != npoints )
  {
    bin_centres.resize(npoints);
  }

  // The custom binary function modifies the behaviour of the algorithm to compute the average of
  // two adjacent bin boundaries
  std::adjacent_difference(bin_edges.begin(), bin_edges.end(), bin_centres.begin(), SimpleAverage<double>());
  // The algorithm copies the first element of the input to the first element of the output so we need to 
  // remove the first element of the output
  bin_centres.erase(bin_centres.begin());
}

//-------------------------------------------------------------------------------------------------
/** Assess if all the values in the vector are equal or if there are some different values
*  @param[in] arra the vector to examine
*  @param[out] val if there is only one value this variable takes that value, otherwise its contents are undefined
*/
bool isConstantValue(const std::vector<double> &arra, double &val)
{
  //make comparisons with the first value
  std::vector<double>::const_iterator i = arra.begin();

  if ( i == arra.end() )
  {//empty array
    return true;
  }

  //this loop can be entered! NAN values make comparisons difficult because nan != nan, deal with these first
  for ( val = *i; val != val ; )
  {
    ++i;
    if ( i == arra.end() )
    {
      //all values are contant (NAN)
      return true;
    }
    val = *i;
  }
  
  for ( ; i != arra.end() ; ++i )
  {
    if ( *i != val )
    {
      return false;
    }
  }
  //no different value was found and so every must be equal to c
  return true;
}

//-------------------------------------------------------------------------------------------------
/** Take a string of comma or space-separated values, and splits it into
 * a vector of doubles.
 * @param listString a string like "0.0 1.2" or "2.4, 5.67, 88"
 * @return a vector of doubles
 * @throw an error if there was a string that could not convert to a double.
 */
std::vector<double> splitStringIntoVector(std::string listString)
{
  //Split the string and turn it into a vector.
  std::vector<double> values;
  std::vector<std::string> strs;
  boost::split(strs, listString, boost::is_any_of(", "));
  for (std::vector<std::string>::iterator it= strs.begin(); it != strs.end(); it++)
  {
    std::stringstream oneNumber(*it);
    double num;
    oneNumber >> num;
    values.push_back(num);
  }
  return values;
}


//-------------------------------------------------------------------------------------------------
/** Return the index into a vector of bin boundaries for a particular X value.
 * The index returned is the one left edge of the bin.
 * If beyond the range of the vector, it will return either 0 or bins.size()-2.
 */
int getBinIndex(std::vector<double>& bins, const double X )
{
  int index = 0;
  //If X is below the min value
  if (X < bins[0])
    return 0;

  int nBins = bins.size();
  for (index = 0; index < nBins-1; index++)
  {
    if ((X >= bins[index]) && (X < bins[index+1]))
      return index;
  }
  //If X is beyond the max value
  return index;
}


} // End namespace VectorHelper
} // End namespace Kernel
} // End namespace Mantid
