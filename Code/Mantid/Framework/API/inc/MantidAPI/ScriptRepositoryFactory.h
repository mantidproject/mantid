#ifndef MANTID_API_SCRIPTREPOSITORYFACTORY_H_
#define MANTID_API_SCRIPTREPOSITORYFACTORY_H_

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
class ScriptRepository;

/** @class ScriptRepositoryFactoryImpl

    The ScriptRepositoryFactoryImpl class is in charge of the creation of
   concrete
    instance of ScriptRepository interface. It inherits most of its
   implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Gesner Passos, ISIS
    @date 20/12/2012

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_API_DLL ScriptRepositoryFactoryImpl
    : public Kernel::DynamicFactory<ScriptRepository> {
public:
private:
  friend struct Mantid::Kernel::CreateUsingNew<ScriptRepositoryFactoryImpl>;

  /// Private Constructor for singleton class
  ScriptRepositoryFactoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  ScriptRepositoryFactoryImpl(const ScriptRepositoryFactoryImpl &);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  ScriptRepositoryFactoryImpl &operator=(const ScriptRepositoryFactoryImpl &);
  /// Private Destructor
  virtual ~ScriptRepositoryFactoryImpl();
};

/// Forward declaration of a specialisation of SingletonHolder for
/// AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<ScriptRepositoryFactoryImpl>;
#endif /* _WIN32 */
typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<
    ScriptRepositoryFactoryImpl> ScriptRepositoryFactory;

} // namespace API
} // namespace Mantid

/**
 * Macro for declaring a new type of function to be used with the
 * FunctionFactory
 */
#define DECLARE_SCRIPTREPOSITORY(classname)                                    \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_function_##classname(            \
      ((Mantid::API::ScriptRepositoryFactory::Instance().subscribe<classname>( \
           #classname)),                                                       \
       0));                                                                    \
  }

#endif /*MANTID_API_SCRIPTREPOSITORYFACTORY_H_*/
