// Includes
#include "MantidAPI/DomainCreatorFactory.h"
#include "MantidAPI/IDomainCreator.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace API {

/**
 * Creates an initialized domain creator
 * @param id :: The id of the creator
 * @param pm :: A pointer to a property manager instance
 * @param workspacePropertyName The name of the workspace property
 * @param domainType
 * @returns A pointer to the new object
 */
IDomainCreator *DomainCreatorFactoryImpl::createDomainCreator(
    const std::string &id, Kernel::IPropertyManager *pm,
    const std::string &workspacePropertyName,
    const unsigned int domainType) const {
  auto creator = this->createUnwrapped(id);
  creator->initialize(pm, workspacePropertyName,
                      (IDomainCreator::DomainType)domainType);
  return creator;
}

//----------------------------------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------------------------------

/**
 * Constructor
 */
DomainCreatorFactoryImpl::DomainCreatorFactoryImpl()
    : Kernel::DynamicFactory<IDomainCreator>() {}

/**
 * Destructor
 */
DomainCreatorFactoryImpl::~DomainCreatorFactoryImpl() {}

} // namespace API
} // namespace Mantid
