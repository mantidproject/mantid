#ifndef MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_
#define MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_
/*WIKI* 

The algorithm takes every event in a [[EventWorkspace]] from detector/time-of-flight space, and converts it into reciprocal space, and places the resulting MDEvents into a [[MDEventWorkspace]].

The conversion can be done either to Q-space in the lab or sample frame, or to HKL of the crystal.

If the OutputWorkspace does NOT already exist, a default one is created. In order to define more precisely the parameters of the [[MDEventWorkspace]], use the [[CreateMDWorkspace]] algorithm first.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidKernel/V3D.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidMDEvents/ConvertToDiffractionMDWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace MDEvents
{

  /** ConvertToDiffractionMDWorkspace :
   * Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace.
   * 
   * @author Janik Zikovsky, SNS
   * @date 2011-03-01 13:14:48.236513
   */
  class DLLExport ConvertToDiffractionMDWorkspace  : public API::Algorithm
  {
  public:
    ConvertToDiffractionMDWorkspace();
    ~ConvertToDiffractionMDWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToDiffractionMDWorkspace";};
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
    MDEvents::MDEventWorkspace3Lean::sptr ws;
    /// Do we clear events on the input during loading?
    bool ClearInputWorkspace;
    /// Perform LorentzCorrection on the fly.
    bool LorentzCorrection;
    /// Map of all the detectors in the instrument
    detid2det_map allDetectors;
    /// Primary flight path (source to sample)
    double l1;
    /// Beam direction and length
    Kernel::V3D beamline;
    /// Path length between source and sample
    double beamline_norm;
    /// Beam direction (unit vector)
    Kernel::V3D beamDir;
    /// Sample position
    Kernel::V3D samplePos;
    /// Progress reporter (shared)
    Kernel::ProgressBase * prog;
    /// Matrix. Multiply this by the lab frame Qx, Qy, Qz to get the desired Q or HKL.
    Kernel::Matrix<double> mat;


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
