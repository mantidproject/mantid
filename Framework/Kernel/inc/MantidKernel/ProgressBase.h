#ifndef MANTID_KERNEL_PROGRESSBASE_H_
#define MANTID_KERNEL_PROGRESSBASE_H_

#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {
//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
class Timer;

/** ProgressBase
 *
 * A base class for progress reporting, to be overridden by Progress for
 *algorithms.
 *
 * @author Janik Zikovsky, SNS
 * @date 2011-03-28 15:57:54.187764
 */
class MANTID_KERNEL_DLL ProgressBase {
public:
  ProgressBase();
  ProgressBase(double start, double end, int64_t numSteps);
  ProgressBase(const ProgressBase &source);
  ProgressBase &operator=(const ProgressBase &rhs);
  virtual ~ProgressBase();

  /// Pure virtual method that does the progress reporting, to be overridden
  virtual void doReport(const std::string &msg = "") = 0;
  /// Override so that the reporter can inform whether a cancellation request
  /// has been used
  virtual bool hasCancellationBeenRequested() const { return false; }

  // ----------------------- Methods shared between progress reporters
  // -----------------------

  //----------------------------------------------------------------------------------------------
  /** Increments the loop counter by 1, then
   * sends the progress notification on behalf of its algorithm.
  */
  void report() {
    // This function was put inline for highest speed.
    if (++m_i - m_last_reported < m_notifyStep)
      return;
    m_last_reported = m_i;
    this->doReport("");
  }

  void report(const std::string &msg);
  void report(int64_t i, const std::string &msg = "");
  void reportIncrement(int inc, const std::string &msg = "");
  void reportIncrement(size_t inc, const std::string &msg = "");
  void setNumSteps(int64_t nsteps);
  void resetNumSteps(int64_t nsteps, double start, double end);
  void setNotifyStep(double notifyStepPct);

  double getEstimatedTime() const;

protected:
  /// Starting progress
  double m_start;
  /// Ending progress
  double m_end;
  /// Loop counter initial value
  int64_t m_ifirst;
  /// Loop counter upper bound
  int64_t m_numSteps;
  /// Frequency of sending the notification (every m_step times)
  int64_t m_notifyStep;
  /// Frequency of sending the notification (as a min percentage step, e.g. 1
  /// for 1 % (default) )
  double m_notifyStepPct;
  /// Progress increment at each loop
  double m_step;
  /// Loop counter
  int64_t m_i;
  /// Last loop counter value the was a peport
  int64_t m_last_reported;
  /// Timer that is started when the progress bar is constructed.
  Kernel::Timer *m_timeElapsed;
  /// Digits of precision in the reporting
  int m_notifyStepPrecision;
};

} // namespace Mantid
} // namespace Kernel

#endif /* MANTID_KERNEL_PROGRESSBASE_H_ */
