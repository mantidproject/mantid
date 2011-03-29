#ifndef MANTID_KERNEL_PROGRESSBASE_H_
#define MANTID_KERNEL_PROGRESSBASE_H_
    
#include "MantidKernel/System.h"


namespace Mantid
{
namespace Kernel
{

  /** ProgressBase
   *
   * A base class for progress reporting, to be overridden by Progress for algorithms.
   * 
   * @author Janik Zikovsky, SNS
   * @date 2011-03-28 15:57:54.187764
   */
  class DLLExport ProgressBase 
  {
  public:
    ProgressBase();
    ProgressBase(double start,double end, int numSteps);
    virtual ~ProgressBase();

    /// Pure virtual method that does the progress reporting, to be overridden
    virtual void doReport(const std::string& msg = "") = 0;

    // ----------------------- Methods shared between progress reporters -----------------------

    //----------------------------------------------------------------------------------------------
    /** Increments the loop counter by 1, then
     * sends the progress notification on behalf of its algorithm.
    */
    void report()
    {
      // This function was put inline for highest speed.
      if (++m_i - m_last_reported < m_notifyStep ) return;
      m_last_reported = m_i;
      this->doReport("");
    }

    void report(const std::string& msg);
    void report(int i, const std::string& msg = "");
    void reportIncrement(int inc, const std::string& msg = "");
    void setNumSteps(int nsteps);
    void setNotifyStep(double notifyStepPct);

  protected:
    /// Starting progress
    double m_start;
    /// Ending progress
    double m_end;
    /// Loop counter initial value
    int m_ifirst;
    /// Loop counter upper bound
    int m_numSteps;
    /// Frequency of sending the notification (every m_step times)
    int m_notifyStep;
    /// Frequency of sending the notification (as a min percentage step, e.g. 1 for 1 % (default) )
    double m_notifyStepPct;
    /// Progress increment at each loop
    double m_step;
    /// Loop counter
    int m_i;
    /// Last loop counter value the was a peport
    int m_last_reported;
  };


} // namespace Mantid
} // namespace Kernel

#endif  /* MANTID_KERNEL_PROGRESSBASE_H_ */
