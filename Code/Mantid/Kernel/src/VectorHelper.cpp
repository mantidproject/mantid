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
 * @param xold - old x array of data
 * @param xnew - new x array of data
 * @param yold - old y array of data
 * @param ynew - new y array of data
 * @param eold - old error array of data
 * @param enew - new error array of data
 * @param distribution - flag defining if distribution data (1) or not (0)
 * @throw runtime_error Thrown if algorithm cannot execute
 * @throw invalid_argument Thrown if input to function is incorrect
 **/
void rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
      const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, bool distribution)
{
  int size_xold = xold.size();
  int size_xnew = xnew.size();
  if (size_xold != static_cast<int> (yold.size() + 1) || size_xold != static_cast<int> (eold.size() + 1))
    throw std::runtime_error("rebin: x,y, and error vectors should be of same size");

  ynew.clear();
  enew.clear();
  ynew.resize(size_xnew - 1); // Make sure y and e vectors are of correct sizes
  enew.resize(size_xnew - 1);

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
 * @param xold - old x array of data
 * @param xnew - new x array of data
 * @param yold - old y array of data
 * @param ynew - new y array of data
 * @param eold - old error array of data
 * @param enew - new error array of data
 * @param addition - if true, rebinned values are added to the existing ynew/enew vectors
 * @throw runtime_error Thrown if vector sizes are inconsistent or if the X vectors do no overlap
 **/
void rebinHistogram(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
    const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew,bool addition)
{
  // Make sure y and e vectors are of correct sizes
  const size_t size_xold = xold.size();
  if (size_xold != (yold.size() + 1) || size_xold != (eold.size() + 1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 longer than x");
  const size_t size_xnew = xnew.size();
  if (size_xnew != (ynew.size() + 1) || size_xnew != (enew.size() + 1))
    throw std::runtime_error("rebin: y and error vectors should be of same size & 1 longer than x");

  // If not adding to existing vectors, make sure ynew & enew contain zeroes
  if (!addition)
  {
    ynew.assign(size_xnew - 1, 0.0);
    enew.assign(size_xnew - 1, 0.0);
  }

  // First find the first Xpoint that is bigger than xnew[0]
  std::vector<double>::const_iterator it = std::upper_bound(xold.begin(), xold.end(), xnew[0]);
  if (it == xold.end())
    throw std::runtime_error("No overlap, max of X-old < min of X-new");
  size_t iold = std::distance(xold.begin(), it); // Where we are now
  size_t inew = 0;
  double frac, fracE;
  double width;
  if (iold == 0)
  {
    frac = 0;
    fracE = 0;
  }
  else
  {
    width = (xold[iold] - xold[iold - 1]);
    frac = yold[iold - 1] / width;
    fracE = std::pow(eold[iold - 1], 2) / width;
  }
  while ((inew + 1) != size_xnew) //Start the loop here
  {
    while (xnew[++inew] < xold[iold]) //Upper xlimit of new vector < upper limit of old vector
    {
      if (iold != 0) // If iold==0, then no counts to add
      {
        width = (xnew[inew] - xnew[inew - 1]);
        ynew[inew - 1] += frac * width; //Increment this
        enew[inew - 1] += fracE * width;
      }
      if ((inew + 1) == size_xnew)
        break;
    }

    if (iold != 0) // Now upper xlimit of new vector > upper limit of old x vector
    {
      width = (xold[iold] - xnew[inew - 1]);
      ynew[inew - 1] += frac * width;
      enew[inew - 1] += fracE * width;
    }

    if ((iold + 1) == size_xold) //Reached the end of old vector
      break;

    while (xold[++iold] < xnew[inew]) // Now increment the upper limit of old vector until it becomes higher than the new one
    {
      ynew[inew - 1] += yold[iold - 1];
      enew[inew - 1] += std::pow(eold[iold - 1], 2);
      if ((iold + 1) == size_xold)
        break;
    }

    if (iold != 0)
    {
      width = (xold[iold] - xold[iold - 1]);
      frac = yold[iold - 1] / width; //Dealing with the new xold
      fracE = std::pow(eold[iold - 1], 2) / width;
    }

    width = (xnew[inew] - xold[iold - 1]);
    ynew[inew - 1] += frac * width;
    enew[inew - 1] += fracE * width;

  }

  if (!addition) //If this used to add at the same time then not necessary.
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
 *  @param xold - old x array of data
 *  @param xnew - new x array of data
 *  @param yold - old y array of data
 *  @param ynew - new y array of data
 *  @param eold - old error array of data
 *  @param enew - new error array of data
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
