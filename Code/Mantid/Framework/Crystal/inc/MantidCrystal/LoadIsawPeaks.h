#ifndef MANTID_CRYSTAL_LOADISAWPEAKS_H_
#define MANTID_CRYSTAL_LOADISAWPEAKS_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
namespace Crystal
{

  /** LoadIsawPeaks : Load an ISAW-style .peaks file
   * into a PeaksWorkspace
   * 
   * @author Janik Zikovsky, SNS
   * @date 2011-03-07 15:22:11.897153
   */
  class DLLExport LoadIsawPeaks  : public API::IDataFileChecker
  {
  public:
    LoadIsawPeaks();
    virtual ~LoadIsawPeaks();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadIsawPeaks";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal;DataHandling\\Isaw";}

    /// do a quick check that this file can be loaded
    bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
    /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
    int fileCheck(const std::string& filePath);
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    std::string  ApplyCalibInfo(std::ifstream                 & in,
                                std::string                     startChar,
                                Geometry::Instrument_const_sptr instr_old,
                                Geometry::Instrument_const_sptr instr,
                                double                          &T0);

    std::string readHeader( Mantid::DataObjects::PeaksWorkspace_sptr outWS, std::ifstream& in,double &T0 );

    void appendFile( Mantid::DataObjects::PeaksWorkspace_sptr outWS, std::string filename);

  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_LOADISAWPEAKS_H_ */
