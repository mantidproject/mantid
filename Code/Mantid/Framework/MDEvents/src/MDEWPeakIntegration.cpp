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
    this->setWikiSummary("Integrate single-crystal peaks in reciprocal space, for [[MDEventWorkspace]]s.");
    this->setOptionalMessage("Integrate single-crystal peaks in reciprocal space, for MDEventWorkspaces.");
    this->setWikiDescription(
        "This algorithm takes two input workspaces: a MDEventWorkspace containing the events in "
        "multi-dimensional space, as well as a PeaksWorkspace containing single-crystal peak locations."
        "\n\n"
        "The PeaksWorkspace will be modified with the integrated intensity and error found being"
        "filled in."
        );
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

    declareProperty(new PropertyWithValue<double>("BackgroundRadius",0.0,Direction::Input),
        "Radius to use to evaluate the background of the peak.\n"
        "The signal density around the peak (PeakRadius < r < BackgroundRadius) is used to estimate the background under the peak.\n"
        "If smaller than PeakRadius, no background measurement is done." );

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::InOut),
        "A PeaksWorkspace containing the peaks to integrate. The peaks' integrated intensities will be updated"
        "with the new values.");
  }

  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param ws ::  MDEventWorkspace to integrate
   */
  template<typename MDE, size_t nd>
  void MDEWPeakIntegration::integrate(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    if (nd != 3)
      throw std::invalid_argument("For now, we expect the input MDEventWorkspace to have 3 dimensions only.");

    /// Peak workspace to integrate
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS = getProperty("PeaksWorkspace");

    /// Value of the CoordinatesToUse property.
    std::string CoordinatesToUse = getPropertyValue("CoordinatesToUse");

    // TODO: Confirm that the coordinates requested match those in the MDEventWorkspace

    /// Radius to use around peaks
    double PeakRadius = getProperty("PeakRadius");
    /// Background radius
    double BackgroundRadius = getProperty("BackgroundRadius");

    PRAGMA_OMP(parallel for schedule(dynamic, 10) )
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
      ws->getBox()->integrateSphere(sphere, PeakRadius*PeakRadius, signal, errorSquared);

      // Integrate around the background radius
      double bgSignal = 0;
      double bgErrorSquared = 0;
      if (BackgroundRadius > PeakRadius)
      {
        ws->getBox()->integrateSphere(sphere, BackgroundRadius*BackgroundRadius, bgSignal, bgErrorSquared);
        // Subtract the peak part to get the intensity in the shell (PeakRadius < r < BackgroundRadius)
        bgSignal -= signal;
        // We can subtract the error (instead of adding) because the two values are 100% dependent; this is the same as integrating a shell.
        bgErrorSquared -= errorSquared;

        double ratio = (PeakRadius / BackgroundRadius);
        // Relative volume of peak vs the background
        double peakVolume = ratio * ratio * ratio;
        // Volume of the bg shell
        double bgVolume = 1.0 - ratio * ratio * ratio;
        // Finally, you will multiply the bg intensity by this to get the estimated background under the peak volume
        double scaleFactor = peakVolume / bgVolume;
        bgSignal *= scaleFactor;
        bgErrorSquared *= scaleFactor;

        // Adjust the integrated values.
        signal -= bgSignal;
        // But we add the errors together
        errorSquared += bgErrorSquared;
      }


      // Save it back in the peak object.
      p.setIntensity(signal);
      p.setSigmaIntensity( sqrt(errorSquared) );

      g_log.information() << "Peak " << i << " at " << pos << ": signal "
          << signal << " (sig^2 " << errorSquared << "), with background "
          << bgSignal << " (sig^2 " << bgErrorSquared << ") subtracted."
          << std::endl;
    }

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MDEWPeakIntegration::exec()
  {
    inWS = getProperty("InputWorkspace");

    CALL_MDEVENT_FUNCTION(this->integrate, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

