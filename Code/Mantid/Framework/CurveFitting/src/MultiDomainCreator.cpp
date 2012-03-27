//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidAPI/JointDomain.h"

namespace Mantid
{
namespace CurveFitting
{

  void MultiDomainCreator::addCreator(const std::string& workspacePropetyName,IDomainCreator* creator)
  {
    m_workspacePropertyNames.push_back(workspacePropetyName);
    m_creators.push_back(boost::shared_ptr<IDomainCreator>(creator));
  }

  /// Create a domain from the input workspace
  void MultiDomainCreator::createDomain(
    const std::string& workspacePropetyName,
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& values, size_t i0)
  {
    (void)workspacePropetyName;
    auto jointDomain = new API::JointDomain;
    for(auto c = m_creators.begin(); c != m_creators.end(); ++c)
    {
      //(**c).createDomain();
      //jointDomain->addDomain();
    }
  }


} // namespace Algorithm
} // namespace Mantid
