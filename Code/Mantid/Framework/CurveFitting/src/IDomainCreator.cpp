//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IDomainCreator.h"
#include "MantidCurveFitting/Fit.h"

namespace Mantid
{
namespace CurveFitting
{

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
