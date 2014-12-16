#ifndef MANTID_MDALGORITHMS_FOREGROUNDMODELFACTORY_H_
#define MANTID_MDALGORITHMS_FOREGROUNDMODELFACTORY_H_
/**
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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidMDAlgorithms/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/ClassMacros.h"

namespace Mantid {
namespace API {
class IFunction;
}
namespace MDAlgorithms {
//
// Forward declaration
//
class ForegroundModel;

/**
 * A factory class for mapping string names of models to their
 * class types. Allows a model to be instantiated from a string.
 */
class MANTID_MDALGORITHMS_DLL ForegroundModelFactoryImpl
    : public Kernel::DynamicFactory<ForegroundModel> {
private:
  /// Base-class type
  typedef Kernel::DynamicFactory<ForegroundModel> BaseClass;

public:
  /// A create method to ensure the model is initialized properly
  ForegroundModel *createModel(const std::string &name,
                               const API::IFunction &fitFunction);

private:
  /// Policy needs to access constructor
  friend struct Kernel::CreateUsingNew<ForegroundModelFactoryImpl>;
  /// Default constructor
  ForegroundModelFactoryImpl();
  DISABLE_COPY_AND_ASSIGN(ForegroundModelFactoryImpl);

  // Do not allow the default create & createUnwrapped to be called
  using BaseClass::create;
  using BaseClass::createUnwrapped;
};

/// Forward declaration of a specialisation of SingletonHolder for
/// ForegroundModelFactoryImpl (needed for dllexport/dllimport).
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_MDALGORITHMS_DLL
    Kernel::SingletonHolder<ForegroundModelFactoryImpl>;
#endif /* _WIN32 */
/// Typedef singleton instance to ForegroundFactory
typedef MANTID_MDALGORITHMS_DLL
    Kernel::SingletonHolder<ForegroundModelFactoryImpl> ForegroundModelFactory;
}
}

#endif /* MANTID_MDALGORITHMS_FOREGROUNDMODELFACTORY_H_ */
