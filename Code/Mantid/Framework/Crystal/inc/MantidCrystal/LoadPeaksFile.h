#ifndef MANTID_CRYSTAL_LOADPEAKSFILE_H_
#define MANTID_CRYSTAL_LOADPEAKSFILE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid
{
namespace Crystal
{

  /** LoadPeaksFile : Load an ISAW-style .peaks file
   * into a PeaksWorkspace
   * 
   * @author Janik Zikovsky, SNS
   * @date 2011-03-07 15:22:11.897153
   */
  class DLLExport LoadPeaksFile  : public API::Algorithm
  {
  public:
    LoadPeaksFile();
    ~LoadPeaksFile();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadPeaksFile";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    std::string readHeader( Mantid::DataObjects::PeaksWorkspace_sptr outWS, std::ifstream& in );

    void appendFile( Mantid::DataObjects::PeaksWorkspace_sptr outWS, std::string filename);

  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_LOADPEAKSFILE_H_ */
