//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidAPI/JointDomain.h"

namespace Mantid
{
namespace CurveFitting
{

  void MultiDomainCreator::setCreator(size_t i, IDomainCreator* creator)
  {
    m_creators[i] = boost::shared_ptr<IDomainCreator>(creator);
  }

  /**
   * Check if i-th creator has been set with setCreator
   * @param i :: Index of a creator, 0 < i < getNCreators()
   */
  bool MultiDomainCreator::hasCreator(size_t i) const
  {
    return static_cast<bool>(m_creators[i]);
  }

  /// Create a domain from the input workspace
  void MultiDomainCreator::createDomain(
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& ivalues, size_t i0)
  {
    if (m_workspacePropertyNames.size() != m_creators.size())
    {
      throw std::runtime_error("Cannot create JointDomain: number of workspaces does not match "
        "the number of creators");
    }
    auto jointDomain = new API::JointDomain;
    API::IFunctionValues_sptr values;
    i0 = 0;
    for(auto c = m_creators.begin(); c != m_creators.end(); ++c)
    {
      if (!(*c))
      {
        throw std::runtime_error("Missing domain creator");
      }
      API::FunctionDomain_sptr domain;
      (**c).createDomain(domain,values,i0);
      jointDomain->addDomain(domain);
      i0 += domain->size();
    }
    domain.reset(jointDomain);
    ivalues = values;
  }


} // namespace Algorithm
} // namespace Mantid
