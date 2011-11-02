#ifndef MANTID_MDEVENTS_CREATEMDWORKSPACE_H_
#define MANTID_MDEVENTS_CREATEMDWORKSPACE_H_
    
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** CreateMDWorkspace :
   *
   * Algorithm to create an empty MDEventWorkspace with a given number of dimensions.
   *
   * 
   * @author Janik Zikovsky
   * @date 2011-02-25 11:54:52.003137
   */
  class DLLExport CreateMDWorkspace  : public BoxControllerSettingsAlgorithm
  {
  public:
    CreateMDWorkspace();
    ~CreateMDWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "CreateMDWorkspace";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init();
    void exec();

    template<typename MDE, size_t nd>
    void finish(typename MDEventWorkspace<MDE, nd>::sptr ws);


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_CREATEMDWORKSPACE_H_ */
