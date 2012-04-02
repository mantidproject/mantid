//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IDomainCreator.h"
#include "MantidCurveFitting/Fit.h"

namespace Mantid
{
namespace CurveFitting
{

  Kernel::Logger& IDomainCreator::log() const
  {
    return static_cast<Fit*>(m_fit)->g_log;
  }

  /**
   * Declare a property to the algorithm.
   * @param prop :: A new property.
   * @param doc :: A doc string.
   */
  void IDomainCreator::declareProperty(Kernel::Property* prop,const std::string& doc)
  {
    static_cast<Fit*>(m_fit)->declareProperty(prop,doc);
  }

  /**
   * Initialize the function with the workspace. Default is to call IFunction::setWorkspace().
   */
  void IDomainCreator::initFunction()
  {
    API::IFunction_sptr function = m_fit->getProperty("Function");
    if (!function)
    {
      throw std::runtime_error("Cannot initialize empty function.");
    }
    API::Workspace_sptr workspace = m_fit->getProperty("InputWorkspace");
    if (!workspace)
    {
      throw std::runtime_error("Cannot initialize function: workspace undefined.");
    }
    function->setWorkspace(workspace);
  }

} // namespace Algorithm
} // namespace Mantid
