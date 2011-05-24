#ifndef MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_
#define MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidGeometry/V3D.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidMDEvents/MakeDiffractionMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace MDEvents
{

  /** MakeDiffractionMDEventWorkspace :
   * Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace.
   * 
   * @author Janik Zikovsky, SNS
   * @date 2011-03-01 13:14:48.236513
   */
  class DLLExport MakeDiffractionMDEventWorkspace  : public API::Algorithm
  {
  public:
    MakeDiffractionMDEventWorkspace();
    ~MakeDiffractionMDEventWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "MakeDiffractionMDEventWorkspace";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init();
    void exec();

    template <class T>
    void convertEventList(int workspaceIndex);

    /// The input event workspace
    DataObjects::EventWorkspace_sptr in_ws;
    /// The output MDEventWorkspace<3>
    MDEvents::MDEventWorkspace3::sptr ws;
    /// Do we clear events on the input during loading?
    bool ClearInputWorkspace;
    /// Perform LorentzCorrection on the fly.
    bool LorentzCorrection;
    /// Map of all the detectors in the instrument
    detid2det_map allDetectors;
    /// Primary flight path (source to sample)
    double l1;
    /// Beam direction and length
    Geometry::V3D beamline;
    /// Path length between source and sample
    double beamline_norm;
    /// Beam direction (unit vector)
    Geometry::V3D beamDir;
    /// Sample position
    Geometry::V3D samplePos;
    /// Progress reporter (shared)
    Kernel::ProgressBase * prog;
    /// Matrix. Multiply this by the lab frame Qx, Qy, Qz to get the desired Q or HKL.
    Geometry::Matrix<double> mat;


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
