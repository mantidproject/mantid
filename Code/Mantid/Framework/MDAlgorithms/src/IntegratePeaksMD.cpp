/*WIKI* 

This algorithm performs integration of single-crystal peaks within a radius (with optional background subtraction) in reciprocal space.

=== Inputs ===

The algorithms takes two input workspaces:

* A MDEventWorkspace containing the events in multi-dimensional space. This would be the output of [[ConvertToDiffractionMDWorkspace]].
* As well as a PeaksWorkspace containing single-crystal peak locations. This could be the output of [[FindPeaksMD]]
* The OutputWorkspace will contain a copy of the input PeaksWorkspace, with the integrated intensity and error found being filled in.

=== Calculations ===

Integration is performed by summing the weights of each MDEvent within the provided radii. Errors are also summed in quadrature.

[[File:IntegratePeaksMD_graph1.png]]

* All the Radii are specified in <math>\AA^{-1}</math>
* A sphere of radius '''PeakRadius''' is integrated around the center of each peak.
** This gives the summed intensity <math>I_{peak}</math> and the summed squared error <math>\sigma I_{peak}^2</math>.
** The volume of integration is <math>V_{peak}</math>.
* If '''BackgroundOuterRadius''' is specified, then a shell, with radius r where '''BackgroundInnerRadius''' < r < '''BackgroundOuterRadius''', is integrated.
** This gives the summed intensity <math>I_{shell}</math> and the summed squared error <math>\sigma I_{shell}^2</math>.
** The volume of integration is <math>V_{shell}</math>.
** '''BackgroundInnerRadius''' allows you to give some space between the peak and the background area.

==== Background Subtraction ====

The background signal within PeakRadius is calculated by scaling the background signal density in the shell to the volume of the peak:

<math>I_{bg} = I_{shell} \frac{V_{peak}}{V_{shell}}</math>

with the error squared on that value:

<math>\sigma I_{bg}^2 = \frac{V_{peak}}{V_{shell}} \sigma I_{shell}^2</math>

This is applied to the integrated peak intensity <math>I_{peak}</math> to give the corrected intensity <math>I_{corr}</math>:

<math>I_{corr} = I_{peak} - I_{bg}</math>

with the errors summed in quadrature:

<math>\sigma I_{corr}^2 = \sigma I_{peak}^2 + \sigma I_{bg}^2 </math>

=== If BackgroundInnerRadius is Omitted ===

If BackgroundInnerRadius is left blank, then '''BackgroundInnerRadius''' = '''PeakRadius''', and the integration is as follows:

[[File:IntegratePeaksMD_graph2.png]]

=== Sample Usage ===

<source lang="python">
# Load a SCD data set and find the peaks
LoadEventNexus(Filename=r'TOPAZ_3131_event.nxs',OutputWorkspace='TOPAZ_3131_nxs')
ConvertToDiffractionMDWorkspace(InputWorkspace='TOPAZ_3131_nxs',OutputWorkspace='TOPAZ_3131_md',LorentzCorrection='1')
FindPeaksMD(InputWorkspace='TOPAZ_3131_md',PeakDistanceThreshold='0.15',MaxPeaks='100',OutputWorkspace='peaks')
FindUBUsingFFT(PeaksWorkspace='peaks',MinD='2',MaxD='16')

# Perform the peak integration, in-place in the 'peaks' workspace.
IntegratePeaksMD(InputWorkspace='TOPAZ_3131_md', PeaksWorkspace='peaks',
    PeakRadius=0.12, BackgroundOuterRadius=0.2, BackgroundInnerRadius=0.16,
    OutputWorkspace='peaks')
</source>

*WIKI*/
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDAlgorithms/IntegratePeaksMD.h"
#include "MantidMDEvents/CoordTransformDistance.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace MDAlgorithms
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
    declareProperty("CoordinatesToUse", "Q (lab frame)",boost::make_shared<StringListValidator>(propOptions),
      "Which coordinates of the peak center do you wish to use to integrate the peak? This should match the InputWorkspace's dimensions."
       );

    declareProperty(new PropertyWithValue<double>("PeakRadius",1.0,Direction::Input),
        "Fixed radius around each peak position in which to integrate (in the same units as the workspace).");

    declareProperty(new PropertyWithValue<double>("BackgroundInnerRadius",0.0,Direction::Input),
        "Inner radius to use to evaluate the background of the peak.\n"
        "If smaller than PeakRadius, then we assume BackgroundInnerRadius = PeakRadius." );

    declareProperty(new PropertyWithValue<double>("BackgroundOuterRadius",0.0,Direction::Input),
        "Outer radius to use to evaluate the background of the peak.\n"
        "The signal density around the peak (BackgroundInnerRadius < r < BackgroundOuterRadius) is used to estimate the background under the peak.\n"
        "If smaller than PeakRadius, no background measurement is done." );


    declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::Input),
        "A PeaksWorkspace containing the peaks to integrate.");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output),
        "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
        "with the peaks' integrated intensities.");

    declareProperty("ReplaceIntensity", true, "Always replace intensity in PeaksWorkspacem (default).\n"
        "If false, then do not replace intensity if calculated value is 0 (used for SNSSingleCrystalReduction)");

    declareProperty("IntegrateIfOnEdge", true, "Only warning if all of peak outer radius is not on detector (default).\n"
        "If false, do not integrate if the outer radius is not on a detector.");

    declareProperty("Cylinder", false, "Default is sphere.  Add next two parameters for cylinder.");

    declareProperty(new PropertyWithValue<double>("CylinderLength",0.0,Direction::Input),
        "Length of cylinder in which to integrate (in the same units as the workspace; 0.0 for sphere).");

    declareProperty(new PropertyWithValue<double>("PercentBackground",0.0,Direction::Input),
        "Percent of CylinderLength that is background (20 is 20%)");

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
    double BackgroundOuterRadius = getProperty("BackgroundOuterRadius");
    /// Start radius of the background
    double BackgroundInnerRadius = getProperty("BackgroundInnerRadius");
    /// Cylinder Length to use around peaks for cylinder
    double cylinderLength = getProperty("CylinderLength");
    double backgroundCylinder = cylinderLength;
    double percentBackground = getProperty("PercentBackground");
    cylinderLength *= 1.0 - (percentBackground/100.);
    /// Replace intensity with 0
    bool replaceIntensity = getProperty("ReplaceIntensity");
    bool integrateEdge = getProperty("IntegrateIfOnEdge");
    bool cylinderBool = getProperty("Cylinder");
    if (BackgroundInnerRadius < PeakRadius)
      BackgroundInnerRadius = PeakRadius;

//
// If the following OMP pragma is included, this algorithm seg faults
// sporadically when processing multiple TOPAZ runs in a script, on 
// Scientific Linux 6.2.  Typically, it seg faults after 2 to 6 runs are 
// processed, though occasionally it will process all 8 requested in the 
// script without crashing.  Since the lower level codes already use OpenMP, 
// parallelizing at this level is only marginally useful, giving about a 
// 5-10% speedup.  Perhaps is should just be removed permanantly, but for 
// now it is commented out to avoid the seg faults.  Refs #5533
//PRAGMA_OMP(parallel for schedule(dynamic, 10) )
    for (int i=0; i < peakWS->getNumberPeaks(); ++i)
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

      // Get the instrument and its detectors
      inst = peakWS->getInstrument();
      // Do not integrate if sphere is off edge of detector
      if (BackgroundOuterRadius > PeakRadius)
      {
        if (!detectorQ(p.getQLabFrame(), BackgroundOuterRadius))
          {
             g_log.warning() << "Warning: sphere for integration is off edge of detector for peak " << i << std::endl;
             if (!integrateEdge)continue;
          }
      }
      else
      {
        if (!detectorQ(p.getQLabFrame(), PeakRadius))
          {
             g_log.warning() << "Warning: sphere for integration is off edge of detector for peak " << i << std::endl;
             if (!integrateEdge)continue;
          }
      }

      // Build the sphere transformation
      bool dimensionsUsed[nd];
      coord_t center[nd];
      for (size_t d=0; d<nd; ++d)
      {
        dimensionsUsed[d] = true; // Use all dimensions
        center[d] = static_cast<coord_t>(pos[d]);
      }
	  signal_t signal = 0;
	  signal_t errorSquared = 0;
	  signal_t bgSignal = 0;
	  signal_t bgErrorSquared = 0;
      if (!cylinderBool)
	  {
			CoordTransformDistance sphere(nd, center, dimensionsUsed);

			// Perform the integration into whatever box is contained within.
			ws->getBox()->integrateSphere(sphere, static_cast<coord_t>(PeakRadius*PeakRadius), signal, errorSquared);

			// Integrate around the background radius

			if (BackgroundOuterRadius > PeakRadius )
			{
				// Get the total signal inside "BackgroundOuterRadius"
				ws->getBox()->integrateSphere(sphere, static_cast<coord_t>(BackgroundOuterRadius*BackgroundOuterRadius), bgSignal, bgErrorSquared);

				// Evaluate the signal inside "BackgroundInnerRadius"
				signal_t interiorSignal = 0;
				signal_t interiorErrorSquared = 0;

				// Integrate this 3rd radius, if needed
				if (BackgroundInnerRadius != PeakRadius)
				{
					ws->getBox()->integrateSphere(sphere, static_cast<coord_t>(BackgroundInnerRadius*BackgroundInnerRadius), interiorSignal, interiorErrorSquared);
				}
				else
				{
					// PeakRadius == BackgroundInnerRadius, so use the previous value
					interiorSignal = signal;
					interiorErrorSquared = errorSquared;
				}
		        // Subtract the peak part to get the intensity in the shell (BackgroundInnerRadius < r < BackgroundOuterRadius)
		        bgSignal -= interiorSignal;
		        // We can subtract the error (instead of adding) because the two values are 100% dependent; this is the same as integrating a shell.
		        bgErrorSquared -= interiorErrorSquared;

		        // Relative volume of peak vs the BackgroundOuterRadius sphere
		        double ratio = (PeakRadius / BackgroundOuterRadius);
		        double peakVolume = ratio * ratio * ratio;

		        // Relative volume of the interior of the shell vs overall backgroundratio * ratio
		        double interiorRatio = (BackgroundInnerRadius / BackgroundOuterRadius);
		        // Volume of the bg shell, relative to the volume of the BackgroundOuterRadius sphere
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
	  }
      else
      {
			CoordTransformDistance cylinder(nd, center, dimensionsUsed, 2);

			// Perform the integration into whatever box is contained within.
			std::vector<signal_t> signal_fit;

			int numSteps = 20;
			double deltaQ = cylinderLength/static_cast<double>(numSteps-1);
			signal_fit.clear();
			for (int j=0; j<numSteps; j++)signal_fit.push_back(0.0);
			ws->getBox()->integrateCylinder(cylinder, static_cast<coord_t>(PeakRadius), static_cast<coord_t>(cylinderLength), signal, errorSquared, signal_fit);
			for (size_t j=0; j<signal_fit.size(); j++)std::cout << signal_fit[j]<<"  ";
			std::cout <<i<<"\n";


			// Integrate around the background radius
			if (BackgroundOuterRadius > PeakRadius || percentBackground > 0.0)
			{
				// Get the total signal inside "BackgroundOuterRadius"

				if (BackgroundOuterRadius < PeakRadius ) BackgroundOuterRadius = PeakRadius;
				int numSteps=static_cast<int>((backgroundCylinder/deltaQ) + 1);
				signal_fit.clear();
				for (int j=0; j<numSteps; j++)signal_fit.push_back(0.0);
				ws->getBox()->integrateCylinder(cylinder, static_cast<coord_t>(BackgroundOuterRadius), static_cast<coord_t>(backgroundCylinder), bgSignal, bgErrorSquared, signal_fit);
				for (size_t j=0; j<signal_fit.size(); j++)std::cout << signal_fit[j]<<"  ";
				std::cout <<i<<"\n";


				// Evaluate the signal inside "BackgroundInnerRadius"
				signal_t interiorSignal = 0;
				signal_t interiorErrorSquared = 0;

				// Integrate this 3rd radius, if needed
				if (BackgroundInnerRadius != PeakRadius)
				{
					ws->getBox()->integrateCylinder(cylinder, static_cast<coord_t>(BackgroundInnerRadius), static_cast<coord_t>(cylinderLength), interiorSignal, interiorErrorSquared, signal_fit);
					for (size_t j=0; j<signal_fit.size(); j++)std::cout << signal_fit[j]<<"  ";
					std::cout <<i<<"\n";
				}
				else
				{
					// PeakRadius == BackgroundInnerRadius, so use the previous value
					interiorSignal = signal;
					interiorErrorSquared = errorSquared;
				}
		        // Subtract the peak part to get the intensity in the shell (BackgroundInnerRadius < r < BackgroundOuterRadius)
		        bgSignal -= interiorSignal;
		        // We can subtract the error (instead of adding) because the two values are 100% dependent; this is the same as integrating a shell.
		        bgErrorSquared -= interiorErrorSquared;
		        // Relative volume of peak vs the BackgroundOuterRadius sphere
		        double ratio = (PeakRadius / BackgroundOuterRadius);
		        double peakVolume = ratio * ratio * (1-percentBackground/100.);

		        // Relative volume of the interior of the shell vs overall backgroundratio * ratio
		        double interiorRatio = (BackgroundInnerRadius / BackgroundOuterRadius);
		        // Volume of the bg shell, relative to the volume of the BackgroundOuterRadius sphere
		        double bgVolume = 1.0 - interiorRatio * interiorRatio * (percentBackground/100.);

		        // Finally, you will multiply the bg intensity by this to get the estimated background under the peak volume
		        double scaleFactor = peakVolume / bgVolume;
		        bgSignal *= scaleFactor;
		        bgErrorSquared *= scaleFactor;
		        // Adjust the integrated values.
		        signal -= bgSignal;
		        // But we add the errors together
		        errorSquared += bgErrorSquared;
			}
      }


      // Save it back in the peak object.
      if (signal != 0. || replaceIntensity)
      {
        p.setIntensity(signal);
        p.setSigmaIntensity( sqrt(errorSquared) );
      }

      g_log.information() << "Peak " << i << " at " << pos << ": signal "
          << signal << " (sig^2 " << errorSquared << "), with background "
          << bgSignal << " (sig^2 " << bgErrorSquared << ") subtracted."
          << std::endl;
    }

    // This flag is used by the PeaksWorkspace to evaluate whether it has been integrated.
    peakWS->mutableRun().addProperty("PeaksIntegrated", 1, true); 
    // These flags are specific to the algorithm.
    peakWS->mutableRun().addProperty("PeakRadius", PeakRadius, true);
    peakWS->mutableRun().addProperty("BackgroundInnerRadius", BackgroundInnerRadius, true);
    peakWS->mutableRun().addProperty("BackgroundOuterRadius", BackgroundOuterRadius, true);

    // Save the output
    setProperty("OutputWorkspace", peakWS);

  }
  /** Calculate if this Q is on a detector
   *
   * @param QLabFrame of radius of integration
   * @param r: Peak radius.
   */
  bool IntegratePeaksMD::detectorQ(Mantid::Kernel::V3D QLabFrame, double r)
  {
    bool in = true;
    int nAngles = 8;
    double dAngles = static_cast<coord_t>(nAngles);
    // check 64 points in theta and phi at outer radius
    for (int i=0; i < nAngles; ++i)
    {
      double theta = 6.28318531/dAngles * i;
      for (int j=0; j < nAngles; ++j)
      {
       double phi = 6.28318531/dAngles * j;
         V3D edge = V3D(QLabFrame.X()+r*std::cos(theta)*std::sin(phi),
           QLabFrame.Y()+r*std::sin(theta)*std::sin(phi), QLabFrame.Z()+r*std::cos(phi));
         // Create the peak using the Q in the lab frame with all its info:
         try
         {
       Peak p(inst, edge);
       in = (in && p.findDetector());
       if (!in) return in;
         }
         catch (...)
         {
           return false;
         }
      }
    }
    return in;
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

