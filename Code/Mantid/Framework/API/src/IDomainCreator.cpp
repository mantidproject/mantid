//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace API
{

  /**
   * @param manager :: A property manager which has information about the data source (eg workspace)
   * and the function.
   * @param workspacePropertyNames :: Property names for workspaces to get the data from.
   * @param domainType :: Type of domain to create: Simple, Sequential, or Parallel.
   */
  IDomainCreator::IDomainCreator( Kernel::IPropertyManager* manager,
    const std::vector<std::string>& workspacePropertyNames,
    DomainType domainType):
  m_manager( manager ),
  m_workspacePropertyNames( workspacePropertyNames ),
    m_domainType( domainType ), m_outputCompositeMembers( false ), m_ignoreInvalidData( false )
  {}

  /**
   * @param value If true then each composite is unrolled and its output is appended to
   * the default output, otherwise just the composite is used
   */
  void IDomainCreator::separateCompositeMembersInOutput(const bool value)
  {
    m_outputCompositeMembers = value;
  }

  /**
   * Declare a property to the algorithm.
   * @param prop :: A new property.
   * @param doc :: A doc string.
   */
  void IDomainCreator::declareProperty(Kernel::Property* prop,const std::string& doc)
  {
    if ( !m_manager ) 
    {
      throw std::runtime_error("IDomainCreator: property manager isn't defined.");
    }
    m_manager->declareProperty(prop,doc);
  }

  /**
   * Initialize the function with the workspace. Default is to call IFunction::setWorkspace().
   */
  void IDomainCreator::initFunction(API::IFunction_sptr function)
  {
    if (!function)
    {
      throw std::runtime_error("IDomainCreator: cannot initialize empty function.");
    }
    if ( !m_manager ) 
    {
      throw std::runtime_error("IDomainCreator: property manager isn't defined.");
    }
    API::Workspace_sptr workspace = m_manager->getProperty("InputWorkspace");
    if (!workspace)
    {
      throw std::runtime_error("IDomainCreator: cannot initialize function: workspace undefined.");
    }
    function->setWorkspace(workspace);
  }

} // namespace Algorithm
} // namespace Mantid
