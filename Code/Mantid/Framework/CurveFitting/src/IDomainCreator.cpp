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
    m_manager->declareProperty(prop,doc);
  }

  /**
   * Initialize the function with the workspace. Default is to call IFunction::setWorkspace().
   */
  void IDomainCreator::initFunction(API::IFunction_sptr function)
  {
    if (!function)
    {
      throw std::runtime_error("Cannot initialize empty function.");
    }
    API::Workspace_sptr workspace = m_manager->getProperty("InputWorkspace");
    if (!workspace)
    {
      throw std::runtime_error("Cannot initialize function: workspace undefined.");
    }
    function->setWorkspace(workspace);
  }

} // namespace Algorithm
} // namespace Mantid
