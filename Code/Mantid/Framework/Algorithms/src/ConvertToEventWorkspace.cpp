#include "MantidAlgorithms/ConvertToEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertToEventWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToEventWorkspace::ConvertToEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertToEventWorkspace::~ConvertToEventWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ConvertToEventWorkspace::initDocs()
  {
    this->setWikiSummary("Converts a Workspace2D from histograms to events in an EventWorkspace by converting each bin to an equivalent weighted event.");
    this->setOptionalMessage("Converts a Workspace2D from histograms to events in an EventWorkspace by converting each bin to an equivalent weighted event.");
    this->setWikiDescription(""
        "This algorithm takes a Workspace2D with any binning or units as its input. "
        "An event is created for each bin of each histogram, except if the bin count is 0.0. "
        "The event is created with an X position (typically time-of-flight) equal to the **center** of the bin. "
        "The weight and error of the event are taken from the histogram value.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertToEventWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input),
        "An input Workspace2D.");
    declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output EventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToEventWorkspace::exec()
  {
    Workspace2D_sptr inWS = getProperty("InputWorkspace");

    //Create the event workspace
    EventWorkspace_sptr outWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inWS->getNumberHistograms(), inWS->blocksize()+1, inWS->blocksize()));

    //Copy geometry, etc. over.
    API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

    Progress prog(this, 0.0, 1.0, inWS->getNumberHistograms());
    PARALLEL_FOR1(inWS)
    for (int iwi=0; iwi<inWS->getNumberHistograms(); iwi++)
    {
      size_t wi = size_t(iwi);
      // The input spectrum (a histogram)
      const ISpectrum * inSpec = inWS->getSpectrum(wi);
      const MantidVec & X = inSpec->dataX();
      const MantidVec & Y = inSpec->dataY();
      const MantidVec & E = inSpec->dataE();
      if (Y.size()+1 != X.size())
        throw std::runtime_error("Expected a histogram (X vector should be 1 longer than the Y vector)");

      // The output event list
      EventList & el = outWS->getEventList(wi);
      // Copy detector IDs and spectra
      el.copyInfoFrom( *inSpec );
      // We need weights but have no way to set the time. So use weighted, no time
      el.switchTo(WEIGHTED_NOTIME);

      for (size_t i=0; i<X.size()-1; i++)
      {
        double weight = Y[i];
        if (weight != 0.0)
        {
          // TOF = midpoint of the bin
          double tof = (X[i] + X[i+1]) / 2.0;
          // Error squared is carried in the event
          double errorSquared = E[i];
          errorSquared *= errorSquared;
          // Create and add the event
          el.addEventQuickly( WeightedEventNoTime(tof, weight, errorSquared) );
        }
      }

      // Set the X binning parameters
      el.setX( inSpec->ptrX() );
      // Manually set that this is sorted by TOF, since it is. This will make it "threadSafe" in other algos.
      el.setSortOrder( TOF_SORT );

      prog.report("Converting");
    }

    // Set the output
    setProperty("OutputWorkspace", outWS);

  }



} // namespace Mantid
} // namespace Algorithms

