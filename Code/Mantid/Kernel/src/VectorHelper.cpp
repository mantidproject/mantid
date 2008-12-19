#include <stdexcept>
#include <cmath>

#include "MantidKernel/VectorHelper.h"

namespace Mantid{
	namespace Kernel{

void rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
      const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, bool distribution)
{
	  int size_xold=xold.size();
	  int size_xnew=xnew.size();
	  if (size_xold!=static_cast<int>(yold.size()+1) || size_xold!=static_cast<int>(eold.size()+1))
		  throw std::runtime_error("rebin: x,y, and error vectors should be of same size");

	  ynew.resize(size_xnew); // Make sure y and e vectors are of correct sizes
	  enew.resize(size_xnew);

      int iold = 0,inew = 0;
      double xo_low, xo_high, xn_low, xn_high, delta(0.0), width;

      while((inew < size_xnew) && (iold < size_xold))
      {
        xo_low = xold[iold];
        xo_high = xold[iold+1];
        xn_low = xnew[inew];
        xn_high = xnew[inew+1];
        if ( xn_high <= xo_low )
          inew++;		/* old and new bins do not overlap */
        else if ( xo_high <= xn_low )
          iold++;		/* old and new bins do not overlap */
        else
        {
          //        delta is the overlap of the bins on the x axis
          //delta = std::min(xo_high, xn_high) - std::max(xo_low, xn_low);
          delta = xo_high<xn_high?xo_high:xn_high;
          delta -= xo_low>xn_low?xo_low:xn_low;
          width = xo_high - xo_low;
          if ( (delta <= 0.0) || (width <= 0.0) )
          {
            throw std::runtime_error("rebin: no bin overlap detected");
          }
          /*
          *        yoldp contains counts/unit time, ynew contains counts
          *	       enew contains counts**2
          *        ynew has been filled with zeros on creation
          */
          if(distribution)
          {
            // yold/eold data is distribution
            ynew[inew] += yold[iold]*delta;
            // this error is calculated in the same way as opengenie
            enew[inew] += eold[iold]*eold[iold]*delta*width;
          }
          else
          {
            // yold/eold data is not distribution
            // do implicit division of yold by width in summing.... avoiding the need for temporary yold array
            // this method is ~7% faster and uses less memory
            ynew[inew] += yold[iold]*delta/width; //yold=yold/width
            // eold=eold/width, so divide by width**2 compared with distribution calculation
            enew[inew] += eold[iold]*eold[iold]*delta/width;
          }
          if ( xn_high > xo_high )
          {
            iold++;
          }
          else
          {
            inew++;
          }
        }
      }

      if(distribution)
      {
        /*
        * convert back to counts/unit time
        */
        for(int i=0; i<size_xnew; ++i)
        {
          {
            width = xnew[i+1]-xnew[i];
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
        for(int i=0; i<size_xnew;++i)
          enew[i]=sqrt(enew[i]);
      }
      return; //without problems
    }


	} // End namespace Kernel
} // End namespace Mantid
