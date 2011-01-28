#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Kernel
{

UnitFactoryImpl::UnitFactoryImpl() :
  DynamicFactory<Unit>(), m_createdUnits(), m_log(Kernel::Logger::get("UnitFactory"))
{
}

UnitFactoryImpl::~UnitFactoryImpl()
{
}

/** Returns an instance of the class with the given name. Overrides the base class method.
 *  If an instance already exists, a pointer to it is returned, otherwise
 *  a new instance is created by the DynamicFactory::create method.
 *  @param className :: The name of the class to be created
 *  @return A shared pointer to the instance of the requested unit
 */
boost::shared_ptr<Unit> UnitFactoryImpl::create(const std::string& className) const
{
  std::map< std::string, boost::shared_ptr<Unit> >::const_iterator it = m_createdUnits.find(className);
  if ( it != m_createdUnits.end() )
  {
    // If an instance has previously been created, just return a pointer to it
    return it->second;
  }
  else
  {
    // Otherwise create & return a new instance and store the pointer in the internal map for next time
    return m_createdUnits[className] = DynamicFactory<Unit>::create(className);
  }
}

} // namespace Kernel
} // namespace Mantid
