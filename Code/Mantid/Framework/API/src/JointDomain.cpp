//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/JointDomain.h"

namespace Mantid
{
namespace API
{
  /// Return the number of points in the domain
  size_t JointDomain::size() const
  {
    size_t n = 0;
    for(auto d = m_domains.begin(); d != m_domains.end(); ++d)
    {
      n += (**d).size();
    };
    return n;
  }
  /// Return the number of parts in the domain
  size_t JointDomain::getNParts() const
  {
    return m_domains.size();
  }
  /// Return i-th domain
  const FunctionDomain& JointDomain::getDomain(size_t i) const
  {
    return *m_domains.at(i);
  }
  void JointDomain::addDomain(FunctionDomain_sptr domain)
  {
    m_domains.push_back(domain);
  }
  
} // namespace API
} // namespace Mantid
