#ifndef  MANTID_KERNEL_VECTORHELPER_H_
#define MANTID_KERNEL_VECTORHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

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
  void rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
        const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, bool distribution);

  struct sumV : public std::unary_function<double, void>
  {
  	sumV():sum(0){}
  	void operator()(double data) {sum+=data;}
  	double sum;
  };
  // Functor for computing variance
  struct varianceV : public std::unary_function<double, void>
  {
  	varianceV(double mean_):var(0),mean(mean_){}
  	void operator()(double data)
  	{
  		data-=mean;
  		var+=(data*data);
  	}
  	double var, mean;
  };

  } // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_VECTORHELPER_H_*/
