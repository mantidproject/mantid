#ifndef MANTID_DATAHANDLING_LOADDSPACEMAP_H_
#define MANTID_DATAHANDLING_LOADDSPACEMAP_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid
{
namespace DataHandling
{

  /** Loads a Dspacemap file (POWGEN binary, VULCAN binary or ascii format) into an OffsetsWorkspace.
   * 
   * @author Janik Zikovsky (code from Vickie Lynch)
   * @date 2011-05-10
   */
  class DLLExport LoadDspacemap  : public API::Algorithm
  {
  public:
    LoadDspacemap();
    ~LoadDspacemap();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadDspacemap";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    void readVulcanAsciiFile(const std::string& fileName, std::map<detid_t,double> & vulcan);
    void readVulcanBinaryFile(const std::string& fileName, std::map<detid_t,double> & vulcan);

    void CalculateOffsetsFromDSpacemapFile(const std::string DFileName,
        Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS);

    void CalculateOffsetsFromVulcanFactors(std::map<detid_t, double> & vulcan,
        Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS);

  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_LOADDSPACEMAP_H_ */
