#ifndef MANTID_DATAHANDLING_LOADVULCANCALFILE_H_
#define MANTID_DATAHANDLING_LOADVULCANCALFILE_H_
    
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
   */
  class DLLExport LoadVulcanCalFile  : public API::Algorithm
  {
  public:
    LoadVulcanCalFile();
    ~LoadVulcanCalFile();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadVulcanCalFile";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const
    {
      return "Loads set of VULCAN's offset files into up to 3 workspaces: a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.";
    }

    /// Algorithm's version for identification 
    virtual int version() const { return 1;}
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling\\Text;Diffraction";}

    void getInstrument3WaysInit();

    /// Read VULCAN's offset file
    void readOffsetFile(DataObjects::OffsetsWorkspace_sptr offsetws, std::string offsetfilename);

    Geometry::Instrument_const_sptr getInstrument();

    static void readCalFile(const std::string& calFileName,
        Mantid::DataObjects::GroupingWorkspace_sptr groupWS, Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
                            Mantid::DataObjects::MaskWorkspace_sptr maskWS);

    
  private:
    
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();
  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_LOADVULCANCALFILE_H_ */
