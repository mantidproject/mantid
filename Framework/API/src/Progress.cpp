//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Progress.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {

namespace {
void checkEnd(double end) {
  if (end > 1.) {
    std::stringstream msg;
    msg << "Progress range invalid. end=" << end;
    throw std::invalid_argument(msg.str());
  }
}
} // namespace

/**
 * Default constructor
 */
Progress::Progress() : ProgressBase(0.0, 1.0, 0), m_alg(nullptr) {}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm *alg, double start, double end, int numSteps)
    : ProgressBase(start, end, int64_t(numSteps)), m_alg(alg) {
  checkEnd(end);
}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm *alg, double start, double end, int64_t numSteps)
    : ProgressBase(start, end, int64_t(numSteps)), m_alg(alg) {
  checkEnd(end);
}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm *alg, double start, double end, size_t numSteps)
    : ProgressBase(start, end, int64_t(numSteps)), m_alg(alg) {
  checkEnd(end);
}

/** Actually do the reporting, without changing the loop counter.
 * This is called by report(), and can be called directly in
 * order to force a report.
 * It can be overridden
 *
 * @param msg
 */
void Progress::doReport(const std::string &msg) {
  // Progress as a float
  // must be between 0 and 1
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
