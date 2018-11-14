// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTIONFACTORY_H_
#define MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTIONFACTORY_H_

#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidMDAlgorithms/DllConfig.h"

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
  using BaseClass = Kernel::DynamicFactory<MDResolutionConvolution>;

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
using MDResolutionConvolutionFactory =
    Kernel::SingletonHolder<MDResolutionConvolutionFactoryImpl>;
} // namespace MDAlgorithms
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_MDALGORITHMS template class MANTID_MDALGORITHMS_DLL Kernel::
    SingletonHolder<Mantid::MDAlgorithms::MDResolutionConvolutionFactoryImpl>;
}
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MDRESOLUTIONCONVOLUTIONFACTORY_H_ */
