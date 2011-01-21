#ifndef MANTID_CURVEFITTING_CONVOLUTION_H_
#define MANTID_CURVEFITTING_CONVOLUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunctionMW.h"
#include <boost/shared_array.hpp>
#include <cmath>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Performes convolution of two functions.


    @author Roman Tolchenov, Tessella plc
    @date 28/01/2010

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport Convolution : public API::CompositeFunctionMW
    {
    public:

      /**
       * Class for helping to read the transformed data. It represent an output of the
       * GSL real fast fourier transform routine. The routine transforms an array of n
       * real numbers into an array of about n/2 complex numbers which are the amplitudes of
       * the positive frequencies of the full complex fourier transform. 
       */
      class HalfComplex
      {
        int m_size;           ///< size of the transformed data
        double* m_data; ///< pointer to the transformed data
        bool m_even;          ///< true if the size of the original data is even
      public:
        /**
         * Constructor. 
         * @param data A pointer to the transformed complex data
         * @param n The size of untransformed real data
         */
        HalfComplex(double* data,const int& n):m_size(n/2+1),m_data(data),m_even(n/2*2==n)
        {
        }
        /// Returns the size of the transform
        int size()const{return m_size;}
        /**
         * The real part of i-th transform coefficient
         * @param i The index of the complex transform coefficient
         * @return The real part 
         */
        double real(int i)const
        {
          if (i >= m_size) return 0.;
          if (i == 0) return m_data[0];
          return m_data[2*i-1];
        }
        /**
         * The imaginary part of i-th transform coefficient
         * @param i The index of the complex transform coefficient
         * @return The imaginary part 
         */
        double imag(int i)const
        {
          if (i >= m_size) return 0.;
          if (i == 0) return 0;
          if (m_even && i == m_size-1) return 0;
          return m_data[2*i];
        }
        /**
         * Set a new value for i-th complex coefficient
         * @param i The index of the coefficient
         * @param re The real part of the new value
         * @param im The imaginary part of the new value
         */
        void set(int i,const double& re,const double& im)
        {
          if (i >= m_size) return;
          if (i == 0)// this is purely real
          {
            m_data[0] = re;
          }
          else if (m_even && i == m_size-1)// this is also purely real
          {
            m_data[2*i-1] = re;
          }
          else
          {
            m_data[2*i-1] = re;
            m_data[2*i] = im;
          }
        }
      };

      /// Constructor
      Convolution();
      /// Destructor
      ~Convolution();

      /// overwrite IFunction base class methods
      std::string name()const{return "Convolution";}
      void function(double* out, const double* xValues, const int& nData)const;
      void functionDeriv(API::Jacobian* out, const double* xValues, const int& nData);

      /// Add a function. 
      int addFunction(IFitFunction* f);

      /// Deletes and zeroes pointer m_resolution forsing function(...) to recalculate the resolution function
      void refreshResolution()const;
    protected:
      /// overwrite IFunction base class method, which declare function parameters
      virtual void init();
      /// Writes itself into a string
      std::string asString()const;

    private:
      /// To keep the Fourier transform of the resolution function (divided by the step in xValues)
      mutable double* m_resolution;
      /// To keep the size of m_resolution
      mutable int m_resolutionSize;
      /// Temporary data storage used in functionDeriv
      mutable boost::shared_array<double> m_tmp;
      /// Temporary data storage used in functionDeriv
      mutable boost::shared_array<double> m_tmp1;
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CONVOLUTION_H_*/
