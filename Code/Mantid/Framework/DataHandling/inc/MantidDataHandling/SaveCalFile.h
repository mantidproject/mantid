#ifndef MANTID_DATAHANDLING_SAVECALFILE_H_
#define MANTID_DATAHANDLING_SAVECALFILE_H_
/*WIKI* 

This algorithm saves an ARIEL-style 5-column ASCII .cal file.

The format is
* Number: ignored.* UDET: detector ID.* Offset: calibration offset. Comes from the OffsetsWorkspace, or 0.0 if none is given.
* Select: 1 if selected (not masked out). Comes from the MaskWorkspace, or 1 if none is given.
* Group: group number. Comes from the GroupingWorkspace, or 1 if none is given.



*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace DataHandling
{

  /** Algorithm to save a 5-column ascii .cal file from  to 3 workspaces:
   * a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.
   * 
   * @author
   * @date 2011-05-10 09:48:31.796980
   */
  class DLLExport SaveCalFile  : public API::Algorithm
  {
  public:
    SaveCalFile();
    ~SaveCalFile();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SaveCalFile";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling";}

    static void saveCalFile(const std::string& calFileName,
        Mantid::DataObjects::GroupingWorkspace_sptr groupWS, Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS, Mantid::API::MatrixWorkspace_sptr maskWS);

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();


  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_SAVECALFILE_H_ */
