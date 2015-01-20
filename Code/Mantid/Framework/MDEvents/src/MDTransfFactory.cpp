#include "MantidMDEvents/MDTransfFactory.h"

namespace Mantid {
namespace MDEvents {

MDTransfFactoryImpl::MDTransfFactoryImpl()
    : Kernel::DynamicFactory<MDTransfInterface>(), m_createdTransf() {}

MDTransfFactoryImpl::~MDTransfFactoryImpl() {}

/** Returns an instance of the class with the given name. Overrides the base
* class method.
*  If an instance already exists, a pointer to it is returned, otherwise
*  a new instance is created by the DynamicFactory::create method.
*  @param className :: The name of the class to be created
*  @return A shared pointer to the instance of the requested MDtransformation
*/
boost::shared_ptr<MDTransfInterface>
MDTransfFactoryImpl::create(const std::string &className) const {
  std::map<std::string, boost::shared_ptr<MDTransfInterface>>::const_iterator
      it = m_createdTransf.find(className);
  if (it != m_createdTransf.end()) {
    // If an instance has previously been created, just return a pointer to it
    return it->second;
  } else {
    // Otherwise create & return a new instance and store the pointer in the
    // internal map for next time
    return m_createdTransf[className] =
               Kernel::DynamicFactory<MDTransfInterface>::create(className);
  }
}

} // namespace MDEvents
} // namespace Mantid
