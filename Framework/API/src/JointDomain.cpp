//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/JointDomain.h"
#include <numeric>

namespace Mantid {
namespace API {
/// Return the overall size the domain which is a sum of sizes of the
/// member domains.
size_t JointDomain::size() const {
  return std::accumulate(m_domains.begin(), m_domains.end(), size_t{0},
                         [](size_t n, const FunctionDomain_sptr &domain) {
                           return n + domain->size();
                         });
}
/// Return the number of parts in the domain
size_t JointDomain::getNParts() const { return m_domains.size(); }
/** Return i-th domain
 * @param i :: An index of a domain.
 */
const FunctionDomain &JointDomain::getDomain(size_t i) const {
  return *m_domains.at(i);
}

/**
 * Add a new domain.
 * @param domain :: A shared pointer to a domain.
 */
void JointDomain::addDomain(FunctionDomain_sptr domain) {
  m_domains.push_back(domain);
}

} // namespace API
} // namespace Mantid
