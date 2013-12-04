#ifndef MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_
#define MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_
    
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Integrate single-crystal peaks in reciprocal-space.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-13 18:11:53.496539
   */
  class DLLExport IntegratePeaksMD2  : public API::Algorithm
  {
  public:
    IntegratePeaksMD2();
    ~IntegratePeaksMD2();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "IntegratePeaksMD";};
    /// Algorithm's version for identification 
    virtual int version() const { return 2;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDAlgorithms";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    template<typename MDE, size_t nd>
    void integrate(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

    /// Input MDEventWorkspace
    Mantid::API::IMDEventWorkspace_sptr inWS;

    /// Calculate if this Q is on a detector
    bool detectorQ(Mantid::Kernel::V3D QLabFrame, double PeakRadius);

    /// Instrument reference
    Geometry::Instrument_const_sptr inst;

  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDALGORITHMS_INTEGRATEPEAKSMD_H_ */
