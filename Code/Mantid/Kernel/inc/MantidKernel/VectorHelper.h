#ifndef MANTID_KERNEL_VECTORHELPER_H_
#define MANTID_KERNEL_VECTORHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <vector>
#include <functional>

namespace Mantid
{
  namespace Kernel
  {
    /** @class VectorHelper VectorHelper.h Kernel/VectorHelper.h



    @author Laurent C Chapon, Rutherford Appleton Laboratory
    @date 16/12/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
  void DLLExport rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
        const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, bool distribution);

  //! Functor used for computing the sum of the square values of a vector, using the accumulate algorithm
  template <class T> struct SumSquares: public std::binary_function<T,T,T>
  {
	  SumSquares(){}
	  T operator()(const T& r, const T& x) const
	  {
		  return (r+x*x);
	  }
  };

  template <class T> struct TimesSquares: public std::binary_function<T,T,T>
    {
  	  TimesSquares(){}
  	  T operator()(const T& l, const T& r) const
  	  {
  		  return (r*r*l*l);
  	  }
    };

  //! Square functor
  template <class T> struct Squares: public std::unary_function<T,T>
  {
    Squares(){}
    T operator()(const T& x) const
    {
  	  return (x*x);
    }
  };

  } // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_VECTORHELPER_H_*/
