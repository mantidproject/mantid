//
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"

namespace Mantid {
namespace MDAlgorithms {
/**
 * Default constructor
 */
MDResolutionConvolutionFactoryImpl::MDResolutionConvolutionFactoryImpl()
    : Kernel::DynamicFactory<MDResolutionConvolution>() {}

/**
 * A create method to ensure the type is initialized properly
 * @param name :: The name of a convolution type
 * @param fgModelName :: The name of the foreground model to pair with the
 * convolution
 * @param fitFunction :: A reference to the function undergoing fit
 */
MDResolutionConvolution *MDResolutionConvolutionFactoryImpl::createConvolution(
    const std::string &name, const std::string &fgModelName,
    const API::IFunctionMD &fitFunction) {
  MDResolutionConvolution *convolution = this->createUnwrapped(name);
  convolution->setFittingFunction(fitFunction);
  convolution->setForegroundModel(fgModelName);
  convolution->initialize();
  return convolution;
}
}
}
