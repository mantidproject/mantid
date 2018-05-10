#ifndef MANTID_API_FUNCMINIMIZERFACTORY_H_
#define MANTID_API_FUNCMINIMIZERFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include <vector>

namespace Mantid {
namespace API {

//----------------------------------------------------------------------
// More forward declarations
//----------------------------------------------------------------------
class IFuncMinimizer;

/** @class FuncMinimizerFactoryImpl

    The FuncMinimizerFactory class is in charge of the creation of concrete
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

class MANTID_API_DLL FuncMinimizerFactoryImpl
    : public Kernel::DynamicFactory<IFuncMinimizer> {
public:
  /// Creates an instance of a minimizer
  boost::shared_ptr<IFuncMinimizer>
  createMinimizer(const std::string &str) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<FuncMinimizerFactoryImpl>;
  /// Private Constructor for singleton class
  FuncMinimizerFactoryImpl();
};

using FuncMinimizerFactory =
    Mantid::Kernel::SingletonHolder<FuncMinimizerFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::FuncMinimizerFactoryImpl>;
}
} // namespace Mantid

/**
 * Macro for declaring a new type of minimizers to be used with the
 * FuncMinimizerFactory
 */
#define DECLARE_FUNCMINIMIZER(classname, username)                             \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_funcminimizer_##classname(       \
      ((Mantid::API::FuncMinimizerFactory::Instance().subscribe<classname>(    \
           #username)),                                                        \
       0));                                                                    \
  }

#endif /*MANTID_API_FUNCMINIMIZERFACTORY_H_*/
