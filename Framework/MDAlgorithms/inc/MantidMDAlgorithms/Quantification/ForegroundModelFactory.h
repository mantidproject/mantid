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
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidMDAlgorithms/DllConfig.h"

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
  using BaseClass = Kernel::DynamicFactory<ForegroundModel>;

public:
  /// A create method to ensure the model is initialized properly
  ForegroundModel *createModel(const std::string &name,
                               const API::IFunction &fitFunction);
  /// Disable copy operator
  ForegroundModelFactoryImpl(const ForegroundModelFactoryImpl &) = delete;

  /// Disable assignment operator
  ForegroundModelFactoryImpl &
  operator=(const ForegroundModelFactoryImpl &) = delete;

private:
  /// Policy needs to access constructor
  friend struct Kernel::CreateUsingNew<ForegroundModelFactoryImpl>;
  /// Default constructor
  ForegroundModelFactoryImpl();

  // Do not allow the default create & createUnwrapped to be called
  using BaseClass::create;
  using BaseClass::createUnwrapped;
};

/// Typedef singleton instance to ForegroundFactory
using ForegroundModelFactory =
    Kernel::SingletonHolder<ForegroundModelFactoryImpl>;
} // namespace MDAlgorithms
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_MDALGORITHMS template class MANTID_MDALGORITHMS_DLL
    Kernel::SingletonHolder<Mantid::MDAlgorithms::ForegroundModelFactoryImpl>;
}
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_FOREGROUNDMODELFACTORY_H_ */
