//
// Includes
//
#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

namespace Mantid {
namespace MDAlgorithms {
/**
 * Default constructor required by the SingletonHolder
 */
ForegroundModelFactoryImpl::ForegroundModelFactoryImpl() {}

/**
 *  A create method to ensure the model is initialized properly
 * @param name :: The name of the model
 * @param fitFunction :: A fit function to reference
 * @return A newly allocated object. Ownership is passed to the caller
 */
ForegroundModel *
ForegroundModelFactoryImpl::createModel(const std::string &name,
                                        const API::IFunction &fitFunction) {
  ForegroundModel *fgModel = this->createUnwrapped(name);
  fgModel->initialize();
  fgModel->setFunctionUnderMinimization(fitFunction);
  return fgModel;
}
}
}
