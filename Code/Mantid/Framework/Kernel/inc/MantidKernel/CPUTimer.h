#ifndef MANTID_KERNEL_CPUTIMER_H_
#define MANTID_KERNEL_CPUTIMER_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"


namespace Mantid
{
namespace Kernel
{

  /** CPUTimer : Timer that uses the CPU time, rather than wall-clock time
   * to measure execution time.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-04 12:17:48.579100
   */
  class DLLExport CPUTimer 
  {
  public:
    CPUTimer();
    ~CPUTimer();

    float elapsed(bool doReset = true);
    void reset();
    float CPUfraction(bool doReset = true);
    std::string str();

  private:
    /// The starting time (implementation dependent format)
    clock_t m_start;

    /// The regular (wall-clock time).
    Timer m_wallClockTime;
  };


  DLLExport std::ostream& operator<<(std::ostream&, CPUTimer&);

} // namespace Mantid
} // namespace Kernel

#endif  /* MANTID_KERNEL_CPUTIMER_H_ */
