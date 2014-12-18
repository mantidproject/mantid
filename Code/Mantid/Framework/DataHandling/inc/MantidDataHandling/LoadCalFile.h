#ifndef MANTID_DATAHANDLING_LOADCALFILE_H_
#define MANTID_DATAHANDLING_LOADCALFILE_H_
    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
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
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Loads a 5-column ASCII .cal file into up to 3 workspaces: a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.";}

    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling\\Text;Diffraction";}

    static void getInstrument3WaysInit(Mantid::API::Algorithm * alg);

    static Geometry::Instrument_const_sptr getInstrument3Ways(API::Algorithm * alg);

    static void readCalFile(const std::string& calFileName,
        Mantid::DataObjects::GroupingWorkspace_sptr groupWS, Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
                            Mantid::DataObjects::MaskWorkspace_sptr maskWS);

    
  private:
    
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    /// Checks if a detector ID is for a monitor on a given instrument
    static bool idIsMonitor(Mantid::Geometry::Instrument_const_sptr inst, int detID);
  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_LOADCALFILE_H_ */
