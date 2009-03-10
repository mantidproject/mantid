//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Progress.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace API
{

/**   Sends the progress notification on bahave of its algorithm
      @param msg Optional message string
  */
void Progress::report(const std::string& msg)
{
    int i = m_i++ - m_ifirst;
    if (i % m_step != 0) return;
    double p = m_start + m_dp*i;
    if (p > m_end) p = m_end;
    m_alg->progress(p,msg);
    m_alg->interruption_point();
}
/**   Sends the progress notification on bahave of its algorithm
      @param i   The new value of the loop counter
      @param msg Optional message string
  */
void Progress::report(int i, const std::string& msg)
{
    m_i = i;
    if (m_i < m_ifirst) m_i = m_ifirst;
    report(msg);
}

} // namespace API
} // namespace Mantid
