#ifndef MANTID_MDEVENTS_CREATEMDWORKSPACE_H_
#define MANTID_MDEVENTS_CREATEMDWORKSPACE_H_
/*WIKI* 

This algorithm creates an empty MDEventWorkspace from scratch. The workspace can have any number of dimensions (up to ~20). Each dimension must have its name, units, extents specified as comma-spearated string.

The SplitInto parameter determines how splitting of dense boxes will be performed. For example, if SplitInto=5 and the number of dimensions is 3, then each box will get split into 5x5x5 sub-boxes.

The SplitThreshold parameter determines how many events to keep in a box before splitting it into sub-boxes. This value can significantly affect performance/memory use! Too many events per box will mean unnecessary iteration and a slowdown in general. Too few events per box will waste memory with the overhead of boxes.

You can create a file-backed MDEventWorkspace by specifying the Filename and Memory parameters.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
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
  class DLLExport CreateMDWorkspace  : public API::Algorithm
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
