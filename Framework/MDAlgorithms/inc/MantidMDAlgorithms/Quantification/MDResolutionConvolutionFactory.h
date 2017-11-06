#ifndef MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTIONFACTORY_H_
#define MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTIONFACTORY_H_
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

namespace Mantid {

namespace API {
class IFunctionMD;
}
namespace MDAlgorithms {
// Forward declarations
class MDResolutionConvolution;

/**
 * Defines a factory for creating convolution types from a string
 */
class MANTID_MDALGORITHMS_DLL MDResolutionConvolutionFactoryImpl
    : public Kernel::DynamicFactory<MDResolutionConvolution> {
private:
  /// Base-class type
  typedef Kernel::DynamicFactory<MDResolutionConvolution> BaseClass;

public:
  /// A create method to ensure the type is initialized properly
  MDResolutionConvolution *
  createConvolution(const std::string &name, const std::string &fgModelName,
                    const API::IFunctionMD &fitFunction);

  /// Disable copy operator
  MDResolutionConvolutionFactoryImpl(
      const MDResolutionConvolutionFactoryImpl &) = delete;

  /// Disable assignment operator
  MDResolutionConvolutionFactoryImpl &
  operator=(const MDResolutionConvolutionFactoryImpl &) = delete;

private:
  /// Policy needs to access constructor
  friend struct Kernel::CreateUsingNew<MDResolutionConvolutionFactoryImpl>;
  /// Default constructor. Private for singleton holder
  MDResolutionConvolutionFactoryImpl();

  // Do not allow the default create & createUnwrapped to be called
  using BaseClass::create;
  using BaseClass::createUnwrapped;
};

/// Typedef singleton instance to MDResolutionConvolutionFactory
typedef Kernel::SingletonHolder<MDResolutionConvolutionFactoryImpl>
    MDResolutionConvolutionFactory;
}
}

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_MDALGORITHMS template class MANTID_MDALGORITHMS_DLL Kernel::
    SingletonHolder<Mantid::MDAlgorithms::MDResolutionConvolutionFactoryImpl>;
}
}

#endif /* MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTIONFACTORY_H_ */
