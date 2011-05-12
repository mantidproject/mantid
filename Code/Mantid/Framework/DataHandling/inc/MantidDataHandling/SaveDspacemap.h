#ifndef MANTID_DATAHANDLING_SAVEDSPACEMAP_H_
#define MANTID_DATAHANDLING_SAVEDSPACEMAP_H_
    
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataHandling
{

  /** Saves an OffsetsWorkspace into a POWGEN-format binary dspace map file.
   * 
   * @author Janik Zikovsky (code from Vickie Lynch)
   * @date 2011-05-12
   */
  class DLLExport SaveDspacemap  : public API::Algorithm
  {
  public:
    SaveDspacemap();
    ~SaveDspacemap();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SaveDspacemap";};
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

    void CalculateDspaceFromCal(Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS, std::string DFileName);

  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_SAVEDSPACEMAP_H_ */
