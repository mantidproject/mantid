#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Timer.h"

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
   m_last_reported(-1),
   m_timeElapsed(new Timer),
   m_notifyStepPrecision(0)
  {
    m_timeElapsed->reset();
  }

  //----------------------------------------------------------------------------------------------
  /** Creates a ProgressBase instance

      @param start :: Starting progress
      @param end :: Ending progress
      @param numSteps :: Number of times report(...) method will be called.
  */
  ProgressBase::ProgressBase(double start,double end, int64_t numSteps)
    : 
    m_start(start), m_end(end),
    m_ifirst(0), m_numSteps(numSteps),
    m_notifyStep(1),
    m_notifyStepPct(1),
    m_step(1), m_i(0),
    m_last_reported(-1),
    m_timeElapsed(new Timer),
    m_notifyStepPrecision(0)
  {
    this->setNumSteps(numSteps);
    m_last_reported = -m_notifyStep;
    m_timeElapsed->reset();
  }
  //----------------------------------------------------------------------------------------------
  /**
   * Copy constructor that builds a new ProgressBase object. The timer state is copied
   * from the other object
   * @param source The source of the copy
   */
  ProgressBase::ProgressBase(const ProgressBase & source) 
    : m_timeElapsed(new Timer) // new object, new timer
  {
    *this = source;
  }

  ProgressBase & ProgressBase::operator=(const ProgressBase & rhs)
  {
    if(this != &rhs)
    {
      m_start = rhs.m_start;
      m_end = rhs.m_end;
      m_ifirst = rhs.m_ifirst;
      m_numSteps = rhs.m_numSteps;
      m_notifyStep = rhs.m_notifyStep;
      m_notifyStepPct = rhs.m_notifyStepPct;
      m_step = rhs.m_step;
      m_i = rhs.m_i;
      m_last_reported = rhs.m_last_reported;
      // copy the timer state, being careful only to copy state & not the actual pointer
      *m_timeElapsed = *rhs.m_timeElapsed;
      m_notifyStepPrecision = rhs.m_notifyStepPrecision;
    }
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ProgressBase::~ProgressBase()
  {
    delete m_timeElapsed;
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
  void ProgressBase::report(int64_t i, const std::string& msg)
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
    m_i += int64_t(inc);
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
    m_i += int64_t(inc);
    if (m_i - m_last_reported < m_notifyStep ) return;
    m_last_reported = m_i;
    this->doReport(msg);
  }


  //----------------------------------------------------------------------------------------------
  /** Change the number of steps between start/end.
   *
   * @param nsteps :: the number of steps to take between start and end
   */
  void ProgressBase::setNumSteps(int64_t nsteps)
  {
    m_numSteps = nsteps;
    if (m_numSteps <= 0) m_numSteps = 1; // Minimum of 1
    m_step = (m_end-m_start) / double(m_numSteps);
    m_notifyStep = (static_cast<int64_t>(double(m_numSteps)*m_notifyStepPct/100/(m_end-m_start)));
    if (m_notifyStep <= 0) m_notifyStep = 1;
  }

  //----------------------------------------------------------------------------------------------
  /** Change the number of steps between start/end.
   *
   * @param nsteps :: the number of steps to take between start and end
   * @param start :: Starting progress
   * @param end :: Ending progress
   */
  void ProgressBase::resetNumSteps(int64_t nsteps, double start, double end)
  {
    m_start = start;
    m_end = end;
    m_i = 0;
    setNumSteps(nsteps);
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
    m_notifyStep = (static_cast<int64_t>(double(m_numSteps)*m_notifyStepPct/100/(m_end-m_start)));
    if (m_notifyStep < 0) m_notifyStep = 1;
    m_notifyStepPrecision = 0;
    if (m_notifyStepPct < 1.0) m_notifyStepPrecision = 1;
    if (m_notifyStepPct < 0.09) m_notifyStepPrecision = 2;
  }


  //----------------------------------------------------------------------------------------------
  /** Returns the estimated number of seconds until the algorithm completes
   *
   * @return seconds estimated to remain. 0 if it cannot calculate it */
  double ProgressBase::getEstimatedTime() const
  {
    double elapsed = double(m_timeElapsed->elapsed_no_reset());
    double prog = double(m_i) * m_step;
    if (prog <= 1e-4)
      return 0.0; // unknown
    else
    {
      double total = elapsed/prog;
      return total - elapsed;
    }
  }

} // namespace Mantid
} // namespace Kernel

