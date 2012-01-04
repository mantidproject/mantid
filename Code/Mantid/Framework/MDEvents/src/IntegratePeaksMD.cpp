/*WIKI* 

This algorithm takes two input workspaces: a MDEventWorkspace containing the events in multi-dimensional space,
as well as a PeaksWorkspace containing single-crystal peak locations.

* A sphere of radius '''PeakRadius''' is integrated around the center of each peak.
* If '''BackgroundRadius''' is specified, then a shell, with radius r where '''BackgroundStartRadius''' < r < '''BackgroundRadius''', is integrated.
** '''BackgroundStartRadius''' allows you to give some space between the peak and the background area.
** '''BackgroundStartRadius''' = '''PeakRadius''' if not specified.

The OutputWorkspace will contain a copy of the input PeaksWorkspace, with the integrated intensity
and error found being filled in.

*WIKI*/
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/IntegratePeaksMD.h"
#include "MantidMDEvents/CoordTransformDistance.h"

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(IntegratePeaksMD)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::MDEvents;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IntegratePeaksMD::IntegratePeaksMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IntegratePeaksMD::~IntegratePeaksMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void IntegratePeaksMD::initDocs()
  {
    this->setWikiSummary("Integrate single-crystal peaks in reciprocal space, for [[MDEventWorkspace]]s.");
    this->setOptionalMessage("Integrate single-crystal peaks in reciprocal space, for MDEventWorkspaces.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void IntegratePeaksMD::init()
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
        "Fixed radius around each peak position in which to integrate (in the same units as the workspace).");

    declareProperty(new PropertyWithValue<double>("BackgroundRadius",0.0,Direction::Input),
        "End radius to use to evaluate the background of the peak.\n"
        "The signal density around the peak (BackgroundStartRadius < r < BackgroundRadius) is used to estimate the background under the peak.\n"
        "If smaller than PeakRadius, no background measurement is done." );

    declareProperty(new PropertyWithValue<double>("BackgroundStartRadius",0.0,Direction::Input),
        "Start radius to use to evaluate the background of the peak.\n"
        "If smaller than PeakRadius, then we assume BackgroundStartRadius = PeakRadius." );

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::Input),
        "A PeaksWorkspace containing the peaks to integrate.");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output),
        "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
        "with the peaks' integrated intensities.");
  }

  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param ws ::  MDEventWorkspace to integrate
   */
  template<typename MDE, size_t nd>
  void IntegratePeaksMD::integrate(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    if (nd != 3)
      throw std::invalid_argument("For now, we expect the input MDEventWorkspace to have 3 dimensions only.");

    /// Peak workspace to integrate
    Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");

    /// Output peaks workspace, create if needed
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
    if (peakWS != inPeakWS)
      peakWS = inPeakWS->clone();

    /// Value of the CoordinatesToUse property.
    std::string CoordinatesToUse = getPropertyValue("CoordinatesToUse");

    // TODO: Confirm that the coordinates requested match those in the MDEventWorkspace

    /// Radius to use around peaks
    double PeakRadius = getProperty("PeakRadius");
    /// Background (end) radius
    double BackgroundRadius = getProperty("BackgroundRadius");
    /// Start radius of the background
    double BackgroundStartRadius = getProperty("BackgroundStartRadius");
    if (BackgroundStartRadius < PeakRadius)
      BackgroundStartRadius = PeakRadius;

    // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 10) )
    for (int i=0; i < int(peakWS->getNumberPeaks()); ++i)
    {
      // Get a direct ref to that peak.
      IPeak & p = peakWS->getPeak(i);

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
      signal_t signal = 0;
      signal_t errorSquared = 0;
      ws->getBox()->integrateSphere(sphere, PeakRadius*PeakRadius, signal, errorSquared);

      // Integrate around the background radius
      signal_t bgSignal = 0;
      signal_t bgErrorSquared = 0;
      if (BackgroundRadius > PeakRadius)
      {
        // Get the total signal inside "BackgroundRadius"
        ws->getBox()->integrateSphere(sphere, BackgroundRadius*BackgroundRadius, bgSignal, bgErrorSquared);

        // Evaluate the signal inside "BackgroundStartRadius"
        signal_t interiorSignal = 0;
        signal_t interiorErrorSquared = 0;

        // Integrate this 3rd radius, if needed
        if (BackgroundStartRadius != PeakRadius)
          ws->getBox()->integrateSphere(sphere, BackgroundStartRadius*BackgroundStartRadius, interiorSignal, interiorErrorSquared);
        else
        {
          // PeakRadius == BackgroundStartRadius, so use the previous value
          interiorSignal = signal;
          interiorErrorSquared = errorSquared;
        }

        // Subtract the peak part to get the intensity in the shell (BackgroundStartRadius < r < BackgroundRadius)
        bgSignal -= interiorSignal;
        // We can subtract the error (instead of adding) because the two values are 100% dependent; this is the same as integrating a shell.
        bgErrorSquared -= interiorErrorSquared;

        // Relative volume of peak vs the BackgroundRadius sphere
        double ratio = (PeakRadius / BackgroundRadius);
        double peakVolume = ratio * ratio * ratio;

        // Relative volume of the interior of the shell vs overall background
        double interiorRatio = (BackgroundStartRadius / BackgroundRadius);
        // Volume of the bg shell, relative to the volume of the BackgroundRadius sphere
        double bgVolume = 1.0 - interiorRatio * interiorRatio * interiorRatio;

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

    // Save the output
    setProperty("OutputWorkspace", peakWS);

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void IntegratePeaksMD::exec()
  {
    inWS = getProperty("InputWorkspace");

    CALL_MDEVENT_FUNCTION(this->integrate, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

