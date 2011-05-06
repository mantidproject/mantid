#ifndef MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Algorithms
{

  /** Creates a new GroupingWorkspace using an instrument from one of:
   *  an input workspace,
   *  an instrument name,
   *  or an instrument IDF file.
   *
   *  Optionally uses bank names to create the groups.");
   * 
   * @author Janik Zikovsky
   * @date 2011-05-02
   */
  class DLLExport CreateGroupingWorkspace  : public API::Algorithm
  {
  public:
    CreateGroupingWorkspace();
    ~CreateGroupingWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "CreateGroupingWorkspace";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "General";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();


  };


} // namespace Mantid
} // namespace Algorithms

#endif  /* MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_ */
