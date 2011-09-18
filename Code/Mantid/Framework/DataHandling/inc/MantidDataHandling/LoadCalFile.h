#ifndef MANTID_DATAHANDLING_LOADCALFILE_H_
#define MANTID_DATAHANDLING_LOADCALFILE_H_
/*WIKI* 

This algorithm loads an ARIEL-style 5-column ASCII .cal file into up to 3 workspaces: a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.

The format is
* Number: ignored.* UDET: detector ID.* Offset: calibration offset. Goes to the OffsetsWorkspace.
* Select: 1 if selected (not masked out). Goes to the MaskWorkspace.
* Group: group number. Goes to the GroupingWorkspace.



*WIKI*/
    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"


namespace Mantid
{
namespace DataHandling
{

  /** Algorithm to load a 5-column ascii .cal file into up to 3 workspaces:
   * a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.
   * 
   * @author Janik Zikovsky
   * @date 2011-05-09
   */
  class DLLExport LoadCalFile  : public API::Algorithm
  {
  public:
    LoadCalFile();
    ~LoadCalFile();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadCalFile";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling";}

    static void getInstrument3WaysInit(Mantid::API::Algorithm * alg);

    static Geometry::Instrument_const_sptr getInstrument3Ways(API::Algorithm * alg);

    static void readCalFile(const std::string& calFileName,
        Mantid::DataObjects::GroupingWorkspace_sptr groupWS, Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS, Mantid::API::MatrixWorkspace_sptr maskWS);

    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    static Kernel::Logger& g_log;    ///< reference to the logger class

  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_LOADCALFILE_H_ */
