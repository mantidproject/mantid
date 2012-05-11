//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SeqDomain.h"

namespace Mantid
{
namespace CurveFitting
{

/// Return the number of points in the domain
size_t SeqDomain::size() const
{
  size_t n = 0;
  for(auto it = m_creators.begin(); it != m_creators.end(); ++it)
  {
    n += (**it).getDomainSize();
  }
  return n;
}

/// Return the number of parts in the domain
size_t SeqDomain::getNDomains() const
{
  return m_creators.size();
}

/**
 * Create and return i-th domain and i-th values, (i-1)th domain is released.
 * @param i :: Index of domain to return.
 * @param domain :: Output pointer to the returned domain.
 * @param values :: Output pointer to the returned values.
 */
void SeqDomain::getDomainAndValues(size_t i, API::FunctionDomain_sptr& domain, API::IFunctionValues_sptr& values) const
{
  if ( i >= m_creators.size() ) throw std::range_error("Function domain index is out of range.");
  if ( !m_domain || i != m_currentIndex )
  {
    m_creators[i]->createDomain(m_domain, m_values);
  }
  m_currentIndex = 0;
}

} // namespace CurveFitting
} // namespace Mantid
