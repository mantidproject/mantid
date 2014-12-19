//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Progress.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {

/**
 * Default constructor
 */
Progress::Progress() : ProgressBase(0.0, 0.0, 0), m_alg(NULL) {}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm *alg, double start, double end, int numSteps)
    : ProgressBase(start, end, int64_t(numSteps)), m_alg(alg) {}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm *alg, double start, double end, int64_t numSteps)
    : ProgressBase(start, end, int64_t(numSteps)), m_alg(alg) {}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm *alg, double start, double end, size_t numSteps)
    : ProgressBase(start, end, int64_t(numSteps)), m_alg(alg) {}

/** Destructor */
Progress::~Progress() {}

/** Actually do the reporting, without changing the loop counter.
 * This is called by report(), and can be called directly in
 * order to force a report.
 * It can be overridden
 *
 * @param msg
 */
void Progress::doReport(const std::string &msg) {
  // Progress as a float
  double p = m_start + m_step * double(m_i - m_ifirst);
  if (p > m_end)
    p = m_end;
  if (!m_alg)
    return;

  m_alg->progress(p, msg, this->getEstimatedTime(),
                  this->m_notifyStepPrecision);
  m_alg->interruption_point();
}

/**
 * @return true if an algorithm has been set & it has requested to be cancelled
 */
bool Progress::hasCancellationBeenRequested() const {
  if (m_alg)
    return m_alg->getCancel();
  else
    return false;
}

} // namespace API
} // namespace Mantid
