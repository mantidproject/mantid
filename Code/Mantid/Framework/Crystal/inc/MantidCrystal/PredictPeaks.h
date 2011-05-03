#ifndef MANTID_CRYSTAL_PREDICTPEAKS_H_
#define MANTID_CRYSTAL_PREDICTPEAKS_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Crystal
{

  /** Using a known crystal lattice and UB matrix, predict where single crystal peaks
   * should be found in detector/TOF space. Creates a PeaksWorkspace containing
   * the peaks at the expected positions.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-29 16:30:52.986094
   */
  class DLLExport PredictPeaks  : public API::Algorithm
  {
  public:
    PredictPeaks();
    ~PredictPeaks();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "PredictPeaks";};
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


  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_PREDICTPEAKS_H_ */
