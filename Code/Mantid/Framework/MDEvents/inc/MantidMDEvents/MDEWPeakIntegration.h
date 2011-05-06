#ifndef MANTID_MDEVENTS_MDEWPEAKINTEGRATION_H_
#define MANTID_MDEVENTS_MDEWPEAKINTEGRATION_H_
    
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** Integrate single-crystal peaks in reciprocal-space.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-13 18:11:53.496539
   */
  class DLLExport MDEWPeakIntegration  : public API::Algorithm
  {
  public:
    MDEWPeakIntegration();
    ~MDEWPeakIntegration();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "MDEWPeakIntegration";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}
    
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

    /// Peak workspace to integrate
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS;

    /// Value of the CoordinatesToUse property.
    std::string CoordinatesToUse;

  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDEWPEAKINTEGRATION_H_ */
