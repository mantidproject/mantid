#ifndef MANTID_API_COSTFUNCTIONFACTORY_H_
#define MANTID_API_COSTFUNCTIONFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {

//----------------------------------------------------------------------
// More forward declarations
//----------------------------------------------------------------------
class ICostFunction;

/** @class CostFunctionFactoryImpl

    The CostFunctionFactory class is in charge of the creation of concrete
    instances of minimizers. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Anders Markvardsen, ISIS, RAL
    @date 20/05/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
*/

class MANTID_API_DLL CostFunctionFactoryImpl
    : public Kernel::DynamicFactory<ICostFunction> {
public:
  /**Creates an instance of a cost function
   * @param type :: The function's type
   * @return A pointer to the created function
   */
  ICostFunction *createFunction(const std::string &type) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<CostFunctionFactoryImpl>;
  /// Private Constructor for singleton class
  CostFunctionFactoryImpl();
};

using CostFunctionFactory =
    Mantid::Kernel::SingletonHolder<CostFunctionFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::CostFunctionFactoryImpl>;
}
}

#endif /*MANTID_API_COSTFUNCTIONFACTORY_H_*/
