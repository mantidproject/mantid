//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidAPI/JointDomain.h"

namespace Mantid
{
namespace CurveFitting
{

  void MultiDomainCreator::setCreator(size_t i, const std::string& workspacePropetyName,IDomainCreator* creator)
  {
    m_workspacePropertyNames[i] = workspacePropetyName;
    m_creators[i] = boost::shared_ptr<IDomainCreator>(creator);
  }

  bool MultiDomainCreator::hasCreator(size_t i) const
  {
    return static_cast<bool>(m_creators[i]);
  }

  /// Create a domain from the input workspace
  void MultiDomainCreator::createDomain(
    const std::vector<std::string>& workspacePropetyNames,
    boost::shared_ptr<API::FunctionDomain>& domain, 
    boost::shared_ptr<API::IFunctionValues>& ivalues, size_t i0)
  {
    if (workspacePropetyNames.size() != m_creators.size())
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
      auto i = static_cast<size_t>(c - m_creators.begin());
      API::FunctionDomain_sptr domain;
      (**c).createDomain(std::vector<std::string>(1,workspacePropetyNames[i]),domain,values,i0);
      jointDomain->addDomain(domain);
      i0 += domain->size();
    }
    domain.reset(jointDomain);
    ivalues = values;
  }


} // namespace Algorithm
} // namespace Mantid
