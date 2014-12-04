#ifndef MANTID_API_FUNCTIONDOMAIN_H_
#define MANTID_API_FUNCTIONDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
//#include "MantidKernel/PropertyManager.h"
#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif
#include <stdexcept>

namespace Mantid
{
namespace API
{
/** Base class that represents the domain of a function.
    It is a generalisation of function arguments. 
    A domain consists at least of a list of function arguments for which a function (IFunction) should 
    be evaluated. 

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL FunctionDomain//: public Kernel::PropertyManager
{
public:
  /// Virtual destructor
  virtual ~FunctionDomain(){}
  /// Return the number of points in the domain
  virtual size_t size() const  = 0;
  /// Reset the the domain so it can be reused. Implement this method for domains with a state.
  virtual void reset() const {}
};

/// typedef for a shared pointer
typedef boost::shared_ptr<FunctionDomain> FunctionDomain_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONDOMAIN_H_*/
