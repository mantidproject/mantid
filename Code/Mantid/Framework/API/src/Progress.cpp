//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Progress.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace API
{

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm* alg,double start,double end, int numSteps)
  : ProgressBase(start, end, numSteps),
    m_alg(alg)
{
}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm* alg,double start,double end, int64_t numSteps)
  : ProgressBase(start, end, int(numSteps)),
    m_alg(alg)
{
}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm* alg,double start,double end, size_t numSteps)
  : ProgressBase(start, end, int(numSteps)),
    m_alg(alg)
{
}

/** Destructor */
Progress::~Progress()
{
}

/** Actually do the reporting, without changing the loop counter.
 * This is called by report(), and can be called directly in
 * order to force a report.
 * It can be overridden
 *
 * @param msg
 */
void Progress::doReport(const std::string& msg)
{
  // Progress as a float
  double p = m_start + m_step*(m_i - m_ifirst);
  if (p > m_end) p = m_end;
  if (!m_alg) return;
  m_alg->progress(p,msg);
  m_alg->interruption_point();
}


} // namespace API
} // namespace Mantid
