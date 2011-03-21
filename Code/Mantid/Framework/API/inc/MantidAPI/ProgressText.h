#ifndef MANTID_API_PROGRESSTEXT_H_
#define MANTID_API_PROGRESSTEXT_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/Progress.h"

namespace Mantid
{
namespace API
{

  /** ProgressText:
   *
   *  A sub-class of Progress, it allows reporting progress to
   *  the console rather than the GUI. For use in testing
   * 
   * @author Janik Zikovsky
   * @date 2011-03-17 10:32:42.157991
   */
  class DLLExport ProgressText : public Progress
  {
  public:
    ProgressText(double start=0.0, double end=1.0, int nsteps=100, bool newLines=true);

    ~ProgressText();
    
    virtual void doReport(const std::string& msg = "");

//    virtual void report(int i,const std::string& msg = "");
//    virtual void reportIncrement(int inc, const std::string& msg = "");

  private:
    /// Use new lines
    bool m_newLines;

    /// Length of the last printed message.
    size_t m_lastMsgLength;

    /// Mutex to avoid mangled output
    Kernel::Mutex coutMutex;
  };


} // namespace Mantid
} // namespace API

#endif  /* MANTID_API_PROGRESSTEXT_H_ */
