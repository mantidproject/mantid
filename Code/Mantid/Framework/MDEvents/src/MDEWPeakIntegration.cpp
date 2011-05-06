#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEWPeakIntegration.h"
#include "MantidMDEvents/CoordTransformDistance.h"

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MDEWPeakIntegration)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::MDEvents;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MDEWPeakIntegration::MDEWPeakIntegration()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDEWPeakIntegration::~MDEWPeakIntegration()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MDEWPeakIntegration::initDocs()
  {
    this->setWikiSummary("Integrate single-crystal peaks in reciprocal space, for MDEventWorkspaces.");
    this->setOptionalMessage("Integrate single-crystal peaks in reciprocal space, for MDEventWorkspaces.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MDEWPeakIntegration::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::InOut),
        "A PeaksWorkspace containing the peaks to integrate. The peaks' integrated intensities will be updated"
        "with the new values.");
  }

  //----------------------------------------------------------------------------------------------
  template<typename MDE, size_t nd>
  void MDEWPeakIntegration::integrate(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    if (nd != 3)
      throw std::invalid_argument("For now, we expect the input MDEventWorkspace to have 3 dimensions only.");

    for (int i=0; i < int(peakWS->getNumberPeaks()); ++i)
    {
      // Get a direct ref to that peak.
      Peak & p = peakWS->getPeak(i);

      // Convert to a position in the dimensions of the workspace
      V3D pos = p.getQLabFrame();

      // Build the sphere transformation
      bool dimensionsUsed[nd];
      CoordType center[nd];
      for (size_t d=0; d<nd; ++d)
      {
        dimensionsUsed[d] = true; // Use all dimensions
        center[d] = pos[d];
      }
      CoordTransformDistance sphere(nd, center, dimensionsUsed);

      // TODO: Determine a radius that makes sense!
      CoordType radius = 1.0;

      // Perform the integration into whatever box is contained within.
      double signal = 0;
      double errorSquared = 0;
      ws->getBox()->integrateSphere(sphere, radius*radius, signal, errorSquared);
    }

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MDEWPeakIntegration::exec()
  {
    inWS = getProperty("InputWorkspace");
//    peakWS = getProperty("PeaksWorkspace");

    CALL_MDEVENT_FUNCTION(this->integrate, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

