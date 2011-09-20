#ifndef MANTID_MDEVENTS_FINDPEAKSMD_H_
#define MANTID_MDEVENTS_FINDPEAKSMD_H_
/*WIKI* 

This algorithm is used to find single-crystal peaks in a multi-dimensional workspace. It looks for high signal density areas, and is based on an algorithm designed by Dennis Mikkelson for ISAW.

The algorithm proceeds in this way:
* Sorts all the boxes in the workspace by decreasing order of signal density (total weighted event sum divided by box volume).
** It will skip any boxes with a density below a threshold. The threshold is <math>TotalSignal / TotalVolume * DensityThresholdFactor</math>.
* The centroid of the strongest box is considered a peak.
* The centroid of the next strongest box is calculated. 
** We look through all the peaks that have already been found. If the box is too close to an existing peak, it is rejected. This distance is PeakDistanceThreshold.
* This is repeated until we find up to MaxPeaks peaks.

Each peak created is placed in the output [[PeaksWorkspace]].
*WIKI*/
    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** FindPeaksMD : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-06-02
   */
  class DLLExport FindPeaksMD  : public API::Algorithm
  {
  public:
    FindPeaksMD();
    ~FindPeaksMD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "FindPeaksMD";};
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

    /// Max # of peaks
    int64_t MaxPeaks;

    /// Progress reporter.
    Mantid::API::Progress * prog;
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_FINDPEAKSMD_H_ */
