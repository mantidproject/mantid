//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/CompositeDomain.h"

#include <boost/lexical_cast.hpp>
#include <set>

namespace Mantid
{
namespace API
{

  /**
   * Associate a member function and a domain. The function will only be applied
   * to this domain.
   * @param funIndex :: Index of a member function.
   * @param domainIndex :: Index of a domain to be associated with the function.
   */
  void MultiDomainFunction::setDomainIndex(size_t funIndex, size_t domainIndex)
  {
    m_domains[funIndex] = std::vector<size_t>(1,domainIndex);
    countNumberOfDomains();
  }

  /**
   * Associate a member function and a list of domains. The function will only be applied
   * to the listed domains.
   * @param funIndex :: Index of a member function.
   * @param domainIndex :: A vector with indices of domains to be associated with the function.
   */
  void MultiDomainFunction::setDomainIndices(size_t funIndex, const std::vector<size_t>& domainIndices)
  {
    m_domains[funIndex] = domainIndices;
    countNumberOfDomains();
  }

  /**
   * Counts number of the domains.
   */
  void MultiDomainFunction::countNumberOfDomains()
  {
    std::set<size_t> dSet;
    for(auto it = m_domains.begin(); it != m_domains.end(); ++it)
    {
      if (it->second.size())
      {
        dSet.insert(it->second.begin(),it->second.end());
      }
    }
    m_nDomains = dSet.size();
    m_maxIndex = dSet.empty() ? 0 : *dSet.rbegin();
  }

  /**
   * Count value offsets for each member domain in a CompositeDomain.
   */
  void MultiDomainFunction::countValueOffsets(const CompositeDomain& domain)const
  {
    m_valueOffsets.clear();
    m_valueOffsets.push_back(0);
    for(size_t i = 0; i < domain.getNParts(); ++i)
    {
      const FunctionDomain& d = domain.getDomain(i);
      m_valueOffsets.push_back(m_valueOffsets.back() + d.size());
    }
  }

  /// Clear all domain indices
  void MultiDomainFunction::clearDomainIndices()
  {
    m_domains.clear();
    countNumberOfDomains();
  }

  /// Function you want to fit to. 
  /// @param domain :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void MultiDomainFunction::function(const FunctionDomain& domain, FunctionValues& values)const
  {
    // works only on CompositeDomain
    if (!dynamic_cast<const CompositeDomain*>(&domain))
    {
      throw std::invalid_argument("Non-CompositeDomain passed to MultiDomainFunction.");
    }
    const CompositeDomain& cd = dynamic_cast<const CompositeDomain&>(domain);
    // domain must not have less parts than m_maxIndex
    if (cd.getNParts() < m_maxIndex)
    {
      throw std::invalid_argument("CompositeDomain has too few parts (" 
        + boost::lexical_cast<std::string>(cd.getNParts()) +
        ") for MultiDomainFunction (max index " + boost::lexical_cast<std::string>(m_maxIndex) + ").");
    }
    // domain and values must be consistent
    if (cd.size() != values.size())
    {
      throw std::invalid_argument("MultiDomainFunction: domain and values have different sizes.");
    }

    // evaluate member functions
    values.zeroCalculated();
    for(size_t iFun = 0; iFun < nFunctions(); ++iFun)
    {
      // find the domains member function must be applied to
      std::vector<size_t> domains;
      auto it = m_domains.find(iFun);
      if (it == m_domains.end())
      {// apply to all domains
        domains.resize(cd.getNParts());
        size_t i = 0;
        std::generate(domains.begin(),domains.end(),[&i]()->size_t{return i++;});
      }
      else
      {// apply to selected domains
        domains.assign(it->second.begin(),it->second.end());
      }

      countValueOffsets(cd);
      for(auto i = domains.begin(); i != domains.end(); ++i)
      {
        const FunctionDomain& d = cd.getDomain(*i);
        FunctionValues tmp(d);
        getFunction(iFun)->function( d, tmp );
        values.addToCalculated(m_valueOffsets[*i],tmp);
      }
    }
  }

  /// Derivatives of function with respect to active parameters
  void MultiDomainFunction::functionDeriv(const FunctionDomain& domain, Jacobian& jacobian)
  {
    // works only on CompositeDomain
    if (!dynamic_cast<const CompositeDomain*>(&domain))
    {
      throw std::invalid_argument("Non-CompositeDomain passed to MultiDomainFunction.");
    }
    const CompositeDomain& cd = dynamic_cast<const CompositeDomain&>(domain);
    // domain must not have less parts than m_maxIndex
    if (cd.getNParts() < m_maxIndex)
    {
      throw std::invalid_argument("CompositeDomain has too few parts (" 
        + boost::lexical_cast<std::string>(cd.getNParts()) +
        ") for MultiDomainFunction (max index " + boost::lexical_cast<std::string>(m_maxIndex) + ").");
    }

    // evaluate member functions derivatives
    for(size_t iFun = 0; iFun < nFunctions(); ++iFun)
    {
      // find the domains member function must be applied to
      std::vector<size_t> domains;
      auto it = m_domains.find(iFun);
      if (it == m_domains.end())
      {// apply to all domains
        domains.resize(cd.getNParts());
        size_t i = 0;
        std::generate(domains.begin(),domains.end(),[&i]()->size_t{return i++;});
      }
      else
      {// apply to selected domains
        domains.assign(it->second.begin(),it->second.end());
      }

      countValueOffsets(cd);
      for(auto i = domains.begin(); i != domains.end(); ++i)
      {
        const FunctionDomain& d = cd.getDomain(*i);
        PartialJacobian J(&jacobian,m_valueOffsets[*i],paramOffset(iFun));
        getFunction(iFun)->functionDeriv(d,J);
      }
    }
  }

} // namespace API
} // namespace Mantid
