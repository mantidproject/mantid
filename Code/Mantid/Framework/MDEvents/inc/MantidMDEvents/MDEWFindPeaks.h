#ifndef MANTID_MDEVENTS_MDEWFINDPEAKS_H_
#define MANTID_MDEVENTS_MDEWFINDPEAKS_H_
    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** MDEWFindPeaks : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-06-02
   */
  class DLLExport MDEWFindPeaks  : public API::Algorithm
  {
  public:
    MDEWFindPeaks();
    ~MDEWFindPeaks();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "MDEWFindPeaks";};
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

    template<typename MDE, size_t nd>
    void findPeaks(typename MDEventWorkspace<MDE, nd>::sptr ws);

    /// Output PeaksWorkspace
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS;

    /// Estimated radius of peaks. Boxes closer than this are rejected
    coord_t peakRadiusSquared;

    /// Thresholding factor
    double DensityThresholdFactor;

  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDEWFINDPEAKS_H_ */
