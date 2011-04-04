#ifndef MANTID_KERNEL_CPUTIMER_H_
#define MANTID_KERNEL_CPUTIMER_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"


namespace Mantid
{
namespace Kernel
{

  /** CPUTimer : TODO: DESCRIPTION
   * 
   * @author
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
    std::string str() const;

  private:
    /// The starting time (implementation dependent format)
    clock_t m_start;

    /// The regular (wall-clock time).
    Timer m_wallClockTime;
  };


} // namespace Mantid
} // namespace Kernel

#endif  /* MANTID_KERNEL_CPUTIMER_H_ */
