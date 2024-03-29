// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDTransfFactory.h"

namespace Mantid::MDAlgorithms {

/** Returns an instance of the class with the given name. Overrides the base
 * class method.
 *  If an instance already exists, a pointer to it is returned, otherwise
 *  a new instance is created by the DynamicFactory::create method.
 *  @param className :: The name of the class to be created
 *  @return A shared pointer to the instance of the requested MDtransformation
 */
std::shared_ptr<MDTransfInterface> MDTransfFactoryImpl::create(const std::string &className) const {
  std::map<std::string, std::shared_ptr<MDTransfInterface>>::const_iterator it = m_createdTransf.find(className);
  if (it != m_createdTransf.end()) {
    // If an instance has previously been created, just return a pointer to it
    return it->second;
  } else {
    // Otherwise create & return a new instance and store the pointer in the
    // internal map for next time
    return m_createdTransf[className] = Kernel::DynamicFactory<MDTransfInterface>::create(className);
  }
}

} // namespace Mantid::MDAlgorithms
