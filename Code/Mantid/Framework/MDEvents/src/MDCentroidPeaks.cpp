#include "MantidMDEvents/MDCentroidPeaks.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MDCentroidPeaks)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MDCentroidPeaks::MDCentroidPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDCentroidPeaks::~MDCentroidPeaks()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MDCentroidPeaks::initDocs()
  {
    this->setWikiSummary("Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to refine their positions.");
    this->setOptionalMessage("Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to refine their positions.");
    this->setWikiDescription(
        "This algorithm starts with a PeaksWorkspace containing the expected positions of peaks in reciprocal space. "
        "It calculates the centroid of the peak by calculating the average of the coordinates of all events within a given "
        "radius of the peak, weighted by the weight (signal) of the event."
        "\n\n"
        "To speed up the calculation, the centroid of the boxes contained within the radius is used (rather than going "
        "through all individual events)."
        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MDCentroidPeaks::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MDCentroidPeaks::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace MDEvents

