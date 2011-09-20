#ifndef MANTID_MDEVENTS_INTEGRATEPEAKSMD_H_
#define MANTID_MDEVENTS_INTEGRATEPEAKSMD_H_
/*WIKI* 

This algorithm takes two input workspaces: a MDEventWorkspace containing the events in multi-dimensional space, as well as a PeaksWorkspace containing single-crystal peak locations.

The PeaksWorkspace will be modified with the integrated intensity and error found beingfilled in.
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

  /** Integrate single-crystal peaks in reciprocal-space.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-13 18:11:53.496539
   */
  class DLLExport IntegratePeaksMD  : public API::Algorithm
  {
  public:
    IntegratePeaksMD();
    ~IntegratePeaksMD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "IntegratePeaksMD";};
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

  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_INTEGRATEPEAKSMD_H_ */
