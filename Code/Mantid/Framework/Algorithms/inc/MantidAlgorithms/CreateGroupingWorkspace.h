#ifndef MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACE_H_
/*WIKI* 

A [[GroupingWorkspace]] is a simple workspace with one value per detector pixel; this value corresponds to the group number that will be used when focussing or summing another workspace.

This algorithm creates a blank GroupingWorkspace. It uses the InputWorkspace, InstrumentName, OR InstrumentFilename parameterto determine which Instrument to create.

If the OldCalFilename parameter is given, the .cal ASCII file will be loaded to fill the group data.

If the GroupNames parameter is given, the names of banks matching the comma-separated strings in the parameter will be used to sequentially number the groups in the output.
*WIKI*/
    
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
