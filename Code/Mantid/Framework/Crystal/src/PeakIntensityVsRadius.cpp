/*WIKI*

Integrates SCD peaks with a range of radii, to plot graphs
of the integrated intensity vs radius.

*WIKI*/

#include "MantidCrystal/PeakIntensityVsRadius.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PeakIntensityVsRadius)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PeakIntensityVsRadius::PeakIntensityVsRadius()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeakIntensityVsRadius::~PeakIntensityVsRadius()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PeakIntensityVsRadius::name() const { return "PeakIntensityVsRadius";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int PeakIntensityVsRadius::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string PeakIntensityVsRadius::category() const { return "Crystal";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PeakIntensityVsRadius::initDocs()
  {
    this->setWikiSummary("Calculate the integrated intensity of peaks vs integration radius.");
    this->setOptionalMessage("Calculate the integrated intensity of peaks vs integration radius.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PeakIntensityVsRadius::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input),
        "An input MDEventWorkspace.");
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::Input),
        "The list of peaks to integrate");

    std::vector<std::string> propOptions;
    propOptions.push_back("Q (lab frame)");
    propOptions.push_back("Q (sample frame)");
    propOptions.push_back("HKL");
    declareProperty("CoordinatesToUse", "Q (lab frame)",new ListValidator(propOptions),
      "Which coordinates of the peak center do you wish to use to integrate the peak? This should match the InputWorkspace's dimensions."
       );

    declareProperty("RadiusStart", 0.0, "Radius at which to start integrating." );
    declareProperty("RadiusEnd", 1.0, "Radius at which to stop integrating." );
    declareProperty("NumSteps", 10, "Number of steps, between start and end, to calculate radius." );

    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "An output workspace2D containing intensity vs radius.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PeakIntensityVsRadius::exec()
  {
    IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");
    PeaksWorkspace_sptr peaksWS = getProperty("PeaksWorkspace");
    double RadiusStart = getProperty("RadiusStart");
    double RadiusEnd = getProperty("RadiusEnd");
    int NumSteps = getProperty("NumSteps");

    // Create a workspace with one spectrum per peak, and one point per radius step
    MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create("Workspace2D", peaksWS->getNumberPeaks(), NumSteps, NumSteps);

    double progStep = 1.0 / double(NumSteps);
    for (int step = 0; step < NumSteps; step++)
    {
      // Step from RadiusStart to RadiusEnd, inclusively
      double radius = RadiusStart + double(step) * (RadiusEnd - RadiusStart) / (double(NumSteps-1));
      g_log.information() << "Integrating radius " << radius << std::endl;

      // Run the integrate algo with this background
      IAlgorithm_sptr alg = this->createSubAlgorithm("IntegratePeaksMD", progStep*double(step), progStep*double(step+1), true /*logging TODO: set false*/);
      alg->setProperty("InputWorkspace", inWS);
      alg->setProperty("PeaksWorkspace", peaksWS);
      alg->setPropertyValue("CoordinatesToUse", this->getPropertyValue("CoordinatesToUse"));
      alg->setProperty("PeakRadius", radius);
      alg->setProperty("BackgroundRadius", 0.);
      alg->setProperty("BackgroundStartRadius", 0.);
      alg->setPropertyValue("OutputWorkspace", "__tmp__PeakIntensityVsRadius");
      alg->execute();
      if (alg->isExecuted())
      {
        // Retrieve the integrated workspace
        PeaksWorkspace_sptr outPeaks = alg->getProperty("OutputWorkspace");
        for (int i=0; i<outPeaks->getNumberPeaks(); i++)
        {
          size_t wi = size_t(i); // workspace index in output
          IPeak & p = outPeaks->getPeak(i);
          outWS->dataX(wi)[step] = radius;
          outWS->dataY(wi)[step] = p.getIntensity();
          outWS->dataE(wi)[step] = p.getSigmaIntensity();
        }
      }
      else
      {
        //TODO: Clear the point
      }
    }

    setProperty("OutputWorkspace", outWS);
  }



} // namespace Mantid
} // namespace Crystal
