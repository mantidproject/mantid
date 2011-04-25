#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** Default constructor
   */
  ProgressBase::ProgressBase()
  :m_start(0), m_end(1.0),
   m_ifirst(0), m_numSteps(1),
   m_notifyStep(1),
   m_notifyStepPct(1),
   m_step(1), m_i(0),
   m_last_reported(-1)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Creates a ProgressBase instance

      @param start :: Starting progress
      @param end :: Ending progress
      @param numSteps :: Number of times report(...) method will be called.
  */
  ProgressBase::ProgressBase(double start,double end, int numSteps)
    : m_start(start),m_end(end),
      m_ifirst(0),
      m_notifyStepPct(1),
      m_i(0)
  {
    this->setNumSteps(numSteps);
    m_last_reported = -m_notifyStep;
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ProgressBase::~ProgressBase()
  {
  }


  //----------------------------------------------------------------------------------------------
  /** Increments the loop counter by 1, then
   * sends the progress notification on behalf of its algorithm.
   *
   * @param msg :: message string that will be displayed in GUI, for example
  */
  void ProgressBase::report(const std::string& msg)
  {
    if (++m_i - m_last_reported < m_notifyStep ) return;
    m_last_reported = m_i;
    this->doReport(msg);
  }


  //----------------------------------------------------------------------------------------------
  /** Sends the progress notification on behalf of its algorithm.
   * Sets the loop counter to a particular value.
   *
      @param i ::   The new value of the loop counter
      @param msg :: Optional message string
  */
  void ProgressBase::report(int i, const std::string& msg)
  {
    // Set the loop coutner to the spot specified.
    m_i = i;
    if (m_i - m_last_reported < m_notifyStep ) return;
    m_last_reported = m_i;
    this->doReport(msg);
  }


  //----------------------------------------------------------------------------------------------
  /** Sends the progress notification and increment the loop counter by more than one.
   *
      @param inc :: Increment the loop counter by this much
      @param msg :: Optional message string
  */
  void ProgressBase::reportIncrement(int inc, const std::string& msg)
  {
    // Increment the loop counter
    m_i += inc;
    if (m_i - m_last_reported < m_notifyStep ) return;
    m_last_reported = m_i;
    this->doReport(msg);
  }


  //----------------------------------------------------------------------------------------------
  /** Sends the progress notification and increment the loop counter by more than one.
   *
      @param inc :: Increment the loop counter by this much
      @param msg :: Optional message string
  */
  void ProgressBase::reportIncrement(size_t inc, const std::string& msg)
  {
    m_i += int(inc);
    if (m_i - m_last_reported < m_notifyStep ) return;
    m_last_reported = m_i;
    this->doReport(msg);
  }


  //----------------------------------------------------------------------------------------------
  /** Change the number of steps between start/end.
   *
   * @param nsteps :: the number of steps to take between start and end
   */
  void ProgressBase::setNumSteps(int nsteps)
  {
    m_numSteps = nsteps;
    if (m_numSteps <= 0) m_numSteps = 1; // Minimum of 1
    m_step = (m_end-m_start) / (m_numSteps);
    m_notifyStep = (static_cast<int>(double(m_numSteps)*m_notifyStepPct/100/(m_end-m_start)));
    if (m_notifyStep <= 0) m_notifyStep = 1;
  }


  //----------------------------------------------------------------------------------------------
  /** Override the frequency at which notifications are sent out.
   * The default is every change of 1%.
   *
   * @param notifyStepPct :: minimum change, in percentage, to skip between updates.
   *        Default is 1..
   */
  void ProgressBase::setNotifyStep(double notifyStepPct)
  {
    m_notifyStepPct = notifyStepPct;
    m_notifyStep = (static_cast<int>(double(m_numSteps)*m_notifyStepPct/100/(m_end-m_start)));
    if (m_notifyStep <= 0) m_notifyStep = 1;
  }



} // namespace Mantid
} // namespace Kernel

