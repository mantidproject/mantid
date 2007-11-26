//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid
{
namespace API
{

Kernel::Logger& WorkspaceFactory::g_log = Kernel::Logger::get("WorkspaceFactory");

// Initialise the instance pointer to zero
WorkspaceFactory* WorkspaceFactory::m_instance = 0;

/// Private constructor for singleton class
WorkspaceFactory::WorkspaceFactory() : Mantid::Kernel::DynamicFactory<Workspace>()
{ }

/** Private destructor
 *  Prevents client from calling 'delete' on the pointer handed 
 *  out by Instance
 */
WorkspaceFactory::~WorkspaceFactory()
{ }

/** A static method which retrieves the single instance of the Algorithm Factory
 * 
 *  @returns A pointer to the factory instance
 */
WorkspaceFactory* WorkspaceFactory::Instance()
{
	if (!m_instance) m_instance=new WorkspaceFactory;
	return m_instance;
}

} // namespace API
} // Namespace Mantid
