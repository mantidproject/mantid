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

    std::vector<std::string> propOptions;
    propOptions.push_back("Q (lab frame)");
    propOptions.push_back("Q (sample frame)");
    propOptions.push_back("HKL");
    declareProperty("CoordinatesToUse", "Q (lab frame)",new ListValidator(propOptions),
      "Which coordinates of the peak center do you wish to use to integrate the peak? This should match the InputWorkspace's dimensions."
       );

    declareProperty(new PropertyWithValue<double>("PeakRadius",1.0,Direction::Input),
        "Fixed radius around each peak position in which to integrate.");

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

    //TODO: PRAGMA_OMP(parallel for schedule(dynamic, 10) )
    for (int i=0; i < int(peakWS->getNumberPeaks()); ++i)
    {
      // Get a direct ref to that peak.
      Peak & p = peakWS->getPeak(i);

      // Get the peak center as a position in the dimensions of the workspace
      V3D pos;
      if (CoordinatesToUse == "Q (lab frame)")
        pos = p.getQLabFrame();
      else if (CoordinatesToUse == "Q (sample frame)")
        pos = p.getQSampleFrame();
      else if (CoordinatesToUse == "HKL")
        pos = p.getHKL();

      double radius = getProperty("PeakRadius");

      // Build the sphere transformation
      bool dimensionsUsed[nd];
      coord_t center[nd];
      for (size_t d=0; d<nd; ++d)
      {
        dimensionsUsed[d] = true; // Use all dimensions
        center[d] = pos[d];
      }
      CoordTransformDistance sphere(nd, center, dimensionsUsed);

      // Perform the integration into whatever box is contained within.
      double signal = 0;
      double errorSquared = 0;
      ws->getBox()->integrateSphere(sphere, radius*radius, signal, errorSquared);

      // Save it back in the peak object.
      p.setIntensity(signal);
      p.setSigmaIntensity( sqrt(errorSquared) );

      g_log.information() << "Peak " << i << " at " << pos << ": signal " << signal << std::endl;
    }

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MDEWPeakIntegration::exec()
  {
    inWS = getProperty("InputWorkspace");
    peakWS = getProperty("PeaksWorkspace");
    CoordinatesToUse = getPropertyValue("CoordinatesToUse");

    CALL_MDEVENT_FUNCTION(this->integrate, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

