/*WIKI* 

This algorithm takes a Workspace2D with any binning or units as its input.
An event is created for each bin of each histogram, except if the bin count is 0.0. The event is created with an X position (typically time-of-flight) equal to the **center** of the bin. The weight and error of the event are taken from the histogram value.

If the GenerateMultipleEvents option is set, then instead of a single event per bin,
a certain number of events evenly distributed along the X bin are generated.
The number of events generated in each bin is calculated by N = (Y/E)^2 .



*WIKI*/
#include "MantidAlgorithms/ConvertToEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include <limits>

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
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertToEventWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input),
        "An input Workspace2D.");
    declareProperty("GenerateMultipleEvents", false,
        "Generate a number of evenly spread events in each bin. See the help for details.\n"
        "Warning! This may use significantly more memory.");
    declareProperty("MaxEventsPerBin", 10,
        "If GenerateMultipleEvents is true, specifies a maximum number of events to generate in a single bin.\n"
        "Use a value that matches your instrument's TOF resolution. Default 10.");
    declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output EventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToEventWorkspace::exec()
  {
    Workspace2D_const_sptr inWS = getProperty("InputWorkspace");

    bool GenerateMultipleEvents = getProperty("GenerateMultipleEvents");
    int MaxEventsPerBin = getProperty("MaxEventsPerBin");

    //Create the event workspace
    EventWorkspace_sptr outWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inWS->getNumberHistograms(), inWS->blocksize()+1, inWS->blocksize()));

    //Copy geometry, etc. over.
    API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, false);

    // Cached values for later checks
    double inf = std::numeric_limits<double>::infinity();
    double ninf = -inf;

    Progress prog(this, 0.0, 1.0, inWS->getNumberHistograms());
    PARALLEL_FOR1(inWS)
    for (int iwi=0; iwi<int(inWS->getNumberHistograms()); iwi++)
    {
      PARALLEL_START_INTERUPT_REGION
      size_t wi = size_t(iwi);
      // The input spectrum (a histogram)
      const ISpectrum * inSpec = inWS->getSpectrum(wi);
      const MantidVec & X = inSpec->readX();
      const MantidVec & Y = inSpec->readY();
      const MantidVec & E = inSpec->readE();
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
        if ((weight != 0.0) && (weight == weight) /*NAN check*/
            && (weight != inf) && (weight != ninf))
        {
          double error = E[i];
          // Also check that the error is not a bad number
          if ((error == error) /*NAN check*/
              && (error != inf) && (error != ninf))
          {
            if (GenerateMultipleEvents)
            {
              // --------- Multiple events per bin ----------
              double errorSquared = error * error;
              // Find how many events to fake
              double val = weight / E[i];
              val *= val;
              // Convert to int with slight rounding up. This is to avoid rounding errors
              int numEvents = int(val + 0.2);
              if (numEvents < 1) numEvents = 1;
              if (numEvents > MaxEventsPerBin) numEvents = MaxEventsPerBin;
              // Scale the weight and error for each
              weight /= numEvents;
              errorSquared /= numEvents;

              // Spread the TOF. e.g. 2 events = 0.25, 0.75.
              double tofStep = (X[i+1] - X[i]) / (numEvents);
              for (size_t j=0; j<size_t(numEvents); j++)
              {
                double tof = X[i] + tofStep * (0.5 + double(j));
                // Create and add the event
                el.addEventQuickly( WeightedEventNoTime(tof, weight, errorSquared) );
              }
            }
            else
            {
              // --------- Single event per bin ----------
              // TOF = midpoint of the bin
              double tof = (X[i] + X[i+1]) / 2.0;
              // Error squared is carried in the event
              double errorSquared = E[i];
              errorSquared *= errorSquared;
              // Create and add the event
              el.addEventQuickly( WeightedEventNoTime(tof, weight, errorSquared) );
            }
          } // error is nont NAN or infinite
        } // weight is non-zero, not NAN, and non-infinite
      } // (each bin)

      // Set the X binning parameters
      el.setX( inSpec->ptrX() );
      // Manually set that this is sorted by TOF, since it is. This will make it "threadSafe" in other algos.
      el.setSortOrder( TOF_SORT );

      prog.report("Converting");
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // Set the output
    setProperty("OutputWorkspace", outWS);

  }



} // namespace Mantid
} // namespace Algorithms

