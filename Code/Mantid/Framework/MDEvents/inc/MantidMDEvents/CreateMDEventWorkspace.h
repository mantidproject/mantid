#ifndef MANTID_MDEVENTS_CREATEMDEVENTWORKSPACE_H_
#define MANTID_MDEVENTS_CREATEMDEVENTWORKSPACE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** CreateMDEventWorkspace :
   *
   * Algorithm to create an empty MDEventWorkspace with a given number of dimensions.
   *
   * 
   * @author Janik Zikovsky
   * @date 2011-02-25 11:54:52.003137
   */
  class DLLExport CreateMDEventWorkspace  : public API::Algorithm
  {
  public:
    CreateMDEventWorkspace();
    ~CreateMDEventWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "CreateMDEventWorkspace";};
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

#endif  /* MANTID_MDEVENTS_CREATEMDEVENTWORKSPACE_H_ */
