#include <stdexcept>
#include <cmath>

#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <iostream>

namespace Mantid
{
namespace Kernel
{
namespace VectorHelper
{

/** Rebins data according to a new output X array
 *
 *  @param xold Old X array of data. 
 *  @param yold Old Y array of data. Must be 1 element shorter than xold.
 *  @param eold Old error array of data. Must be same length as yold.
 *  @param xnew X array of data to rebin to.
 *  @param ynew Rebinned data. Must be 1 element shorter than xnew.
 *  @param enew Rebinned errors. Must be same length as ynew.
 *  @param distribution Flag defining if distribution data (true) or not (false).
 *  @throw runtime_error Thrown if algorithm cannot execute.
 *  @throw invalid_argument Thrown if input to function is incorrect.
 **/
void rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
      const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, bool distribution)
{
  // Make sure y and e vectors are of correct sizes
  const size_t size_xold = xold.size();
  if (size_xold != (yold.size() + 1) || size_xold != (eold.size() + 1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");
  const size_t size_xnew = xnew.size();
  if (size_xnew != (ynew.size() + 1) || size_xnew != (enew.size() + 1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 shorter than x");

  // Make sure ynew & enew contain zeroes
  ynew.assign(size_xnew - 1, 0.0);
  enew.assign(size_xnew - 1, 0.0);

  int iold = 0, inew = 0;
  double xo_low, xo_high, xn_low, xn_high, delta(0.0), width;
  int size_yold = yold.size();
  int size_ynew = ynew.size();

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
        throw std::runtime_error("rebin: no bin overlap detected");
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
    //non distribution , just square root final error value
    for (int i = 0; i < size_ynew; ++i)
      enew[i] = sqrt(enew[i]);
  }
  return; //without problems
}

/** Rebins histogram data according to a new output X array. Should be faster than previous one.
 *  @author Laurent Chapon 10/03/2009
 *
 *  @param xold Old X array of data. 
 *  @param yold Old Y array of data. Must be 1 element shorter than xold.
 *  @param eold Old error array of data. Must be same length as yold.
 *  @param xnew X array of data to rebin to.
 *  @param ynew Rebinned data. Must be 1 element shorter than xnew.
 *  @param enew Rebinned errors. Must be same length as ynew.
 *  @param addition If true, rebinned values are added to the existing ynew/enew vectors.
 *  @throw runtime_error Thrown if vector sizes are inconsistent or if the X vectors do no overlap
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
    if (it == xold.end())
      throw std::runtime_error("No overlap: max of X-old < min of X-new");
    iold = std::distance(xold.begin(), it) - 1; // Old bin to start at (counting from 0)
  }
  else
  {
    std::vector<double>::const_iterator it = std::upper_bound(xnew.begin(), xnew.end(), xold.front());
    if (it == xnew.end())
      throw std::runtime_error("No overlap: max of X-new < min of X-old");
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

/** Rebins the data according to new output X array.
 *  Values in the input vectors are placed in the new vector as follows:
 *  <UL>
 *  <LI> No overlap between old and new bins: 0 (as regular rebin). </LI>
 *  <LI> New bin entirely within old bin range: Full value of input bin copied. </LI>
 *  <LI> New bin partially covered by old bin: Old value scaled according to fraction of new bin covered by old bin. </LI>
 *  </UL>  
 *
 *  @param xold Old X array of data
 *  @param yold Old Y array of data
 *  @param eold Old error array of data
 *  @param xnew X array of data to rebin to.
 *  @param ynew Rebinned data. Must be 1 element shorter than xnew.
 *  @param enew Rebinned errors. Must be same length as ynew.
 *  @param addition - if true, rebinned values are added to the existing ynew/enew vectors
 *  @throw std::runtime_error If the vector sizes are inconsistent
 **/
void rebinNonDispersive(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
  const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, bool addition)
{
  // Make sure y and e vectors are of correct sizes
  const size_t size_xold=xold.size();
  if (size_xold!=(yold.size()+1) || size_xold!=(eold.size()+1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 longer than x");
  const size_t size_xnew=xnew.size();
  if (size_xnew!=(ynew.size()+1) || size_xnew!=(enew.size()+1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 longer than x");
  
  // If not adding to existing vectors, make sure ynew & enew contain zeroes
  if (!addition)
  {
    ynew.assign(size_xnew-1,0.0);
    enew.assign(size_xnew-1,0.0);
  }
  
  int iold = 0,inew = 0;
  double xo_low, xo_high, xn_low, xn_high;
  int size_yold=yold.size();
  int size_ynew=ynew.size();

  while((inew < size_ynew) && (iold < size_yold))
  {
    xo_low = xold[iold];
    xo_high = xold[iold+1];
    xn_low = xnew[inew];
    xn_high = xnew[inew+1];
    if ( xn_high <= xo_low )
    {
      inew++;   /* old and new bins do not overlap */
    }
    else if ( xo_high <= xn_low )
    {
      iold++;   /* old and new bins do not overlap */
    }
    else if ( xn_low < xo_low && xo_low < xn_high )
    {
      // If the new bin is partially overlapped by the old then scale the value
      const double delta = (xn_high-xo_low)/(xn_high-xn_low);
      ynew[inew] += yold[iold]*delta;
      enew[inew] += eold[iold]*delta;
      ++inew;
    }
    else if ( xn_low < xo_high && xo_high < xn_high )
    {
      // If the new bin is partially overlapped by the old then scale the value
      const double delta = (xo_high-xn_low)/(xn_high-xn_low);
      ynew[inew] += yold[iold]*delta;
      enew[inew] += eold[iold]*delta;
      ++iold;
    }
    else
    {
      // Other wise the new bin is entirely within the old one and we just copy the value
      ynew[inew] += yold[iold];
      enew[inew] += eold[iold];

      if ( xn_high > xo_high )
      {
        iold++;
      }
      
      inew++;
    }
  }

  return; //without problems
}

} // End namespace VectorHelper
} // End namespace Kernel
} // End namespace Mantid
