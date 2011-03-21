//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Progress.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace API
{

/** Creates a Progress instance.
*/
Progress::Progress()
  :m_alg(NULL), m_start(0), m_end(1.0),
   m_ifirst(0), m_numSteps(1),
   m_notifyStep(1),
   m_notifyStepPct(1),
   m_step(1), m_i(0),
   m_last_reported(-1)
{
}

/** Creates a Progress instance
    @param alg :: Algorithm reporting its progress
    @param start :: Starting progress
    @param end :: Ending progress
    @param numSteps :: Number of times report(...) method will be called.
*/
Progress::Progress(Algorithm* alg,double start,double end, int numSteps)
  :m_alg(alg),
   m_start(start),m_end(end),
   m_ifirst(0),
   m_notifyStepPct(1),
   m_i(0)
{
  this->setNumSteps(numSteps);
  m_last_reported = -m_notifyStep;
}

/// Destructor
Progress::~Progress()
{}


/** Actually do the reporting, without changing the loop counter.
 * This is called by report(), and can be called directly in
 * order to force a report.
 * It can be overridden
 *
 * @param msg
 */
void Progress::doReport(const std::string& msg)
{
  double p = m_start + m_step*(m_i - m_ifirst);
  if (p > m_end) p = m_end;
  if (!m_alg) return;
  m_alg->progress(p,msg);
  m_alg->interruption_point();
  m_last_reported = m_i;
}

/** Increments the loop counter by 1, then
 * sends the progress notification on behalf of its algorithm.
 *
 * @param msg :: Optional message string
*/
void Progress::report(const std::string& msg)
{
  if (m_i++ - m_last_reported < m_notifyStep ) return;
  this->doReport(msg);
}

/** Sends the progress notification on behalf of its algorithm
    @param i ::   The new value of the loop counter
    @param msg :: Optional message string
*/
void Progress::report(int i, const std::string& msg)
{
  m_i = i;
  if (m_i < m_ifirst) m_i = m_ifirst;
  report(msg);
}


/** Sends the progress notification and increment it by more than one.
 *
    @param inc :: Increment the loop counter by this much
    @param msg :: Optional message string
*/
void Progress::reportIncrement(int inc, const std::string& msg)
{
  m_i += (inc-1); // report will also increment by 1!
  report(msg);
}


/** Change the number of steps between start/end.
 *
 * @param nsteps :: the number of steps to take between start and end
 */
void Progress::setNumSteps(int nsteps)
{
  m_numSteps = nsteps;
  if (m_numSteps <= 0) m_numSteps = 1; // Minimum of 1
  m_step = (m_end-m_start) / (m_numSteps);
  m_notifyStep = (static_cast<int>(double(m_numSteps)*m_notifyStepPct/100/(m_end-m_start)));
  if (m_notifyStep <= 0) m_notifyStep = 1;
}


/** Override the frequency at which notifications are sent out.
 * The default is every change of 1%.
 *
 * @param notifyStepPct :: minimum change, in percentage, to skip between updates.
 *        Default is 1..
 */
void Progress::setNotifyStep(double notifyStepPct)
{
  m_notifyStepPct = notifyStepPct;
  m_notifyStep = (static_cast<int>(double(m_numSteps)*m_notifyStepPct/100/(m_end-m_start)));
  if (m_notifyStep <= 0) m_notifyStep = 1;
}

} // namespace API
} // namespace Mantid
