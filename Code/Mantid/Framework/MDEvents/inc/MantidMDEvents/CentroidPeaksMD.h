#ifndef MANTID_MDEVENTS_MDCENTROIDPEAKS_H_
#define MANTID_MDEVENTS_MDCENTROIDPEAKS_H_
/*WIKI* 

This algorithm starts with a PeaksWorkspace containing the expected positions of peaks in reciprocal space. It calculates the centroid of the peak by calculating the average of the coordinates of all events within a given radius of the peak, weighted by the weight (signal) of the event.


*WIKI*/
    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to refine their positions.
   * 
   * @author Janik Zikovsky
   * @date 2011-06-01
   */
  class DLLExport CentroidPeaksMD  : public API::Algorithm
  {
  public:
    CentroidPeaksMD();
    ~CentroidPeaksMD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "CentroidPeaksMD";};
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
    void integrate(typename MDEventWorkspace<MDE, nd>::sptr ws);

    /// Input MDEventWorkspace
    Mantid::API::IMDEventWorkspace_sptr inWS;



  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDCENTROIDPEAKS_H_ */
