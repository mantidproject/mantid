#include "MantidMDEvents/MDEWFindPeaks.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <vector>
#include <map>
#include "MantidGeometry/IInstrument.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IInstrument_sptr;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MDEWFindPeaks)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MDEWFindPeaks::MDEWFindPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDEWFindPeaks::~MDEWFindPeaks()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MDEWFindPeaks::initDocs()
  {
    this->setWikiSummary("Find peaks in reciprocal space in a MDEventWorkspace.");
    this->setOptionalMessage("Find peaks in reciprocal space in a MDEventWorkspace.");
    this->setWikiDescription(""
        "This algorithm is used to find single-crystal peaks in a multi-dimensional workspace. It looks for high signal density areas, and is based on an algorithm designed by Dennis Mikkelson for ISAW."
        "\n\n"
        "The algorithm proceeds in this way:\n"
        "* Sorts all the boxes in the workspace by decreasing order of signal density (total weighted event sum divided by box volume).\n"
        "** It will skip any boxes with a density below a threshold. The threshold is <math>TotalSignal / TotalVolume * DensityThresholdFactor</math>.\n"
        "* The centroid of the strongest box is considered a peak.\n"
        "* The centroid of the next strongest box is calculated. \n"
        "** We look through all the peaks that have already been found. If the box is too close to an existing peak, it is rejected. This distance is PeakDistanceThreshold.\n"
        "* This is repeated until we find up to MaxPeaks peaks."
        "\n\n"
        "Each peak created is placed in the output [[PeaksWorkspace]]."

        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MDEWFindPeaks::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input),
        "An input MDEventWorkspace with at least 3 dimensions.");

    declareProperty(new PropertyWithValue<double>("PeakDistanceThreshold",1.0,Direction::Input),
        "Threshold distance for rejecting peaks that are found to be too close from each other.\n"
        "This should be some multiple of the radius of a peak."
        );

    declareProperty(new PropertyWithValue<int64_t>("MaxPeaks",1000,Direction::Input),
        "Maximum number of peaks to find."
        );

    declareProperty(new PropertyWithValue<double>("DensityThresholdFactor", 1.0, Direction::Input),
        "The overall signal density of the workspace will be multiplied by this factor "
        "to get a threshold signal density below which boxes are NOT considered to be peaks. See the help."
        );

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output),
        "An output PeaksWorkspace with the peaks' found positions.");
  }


  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param ws ::  MDEventWorkspace to integrate
   */
  template<typename MDE, size_t nd>
  void MDEWFindPeaks::findPeaks(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    if (nd < 3)
      throw std::invalid_argument("Workspace must have at least 3 dimensions.");

    prog->report("Refreshing Centroids");

    // TODO: This might be slow, progress report?
    // Make sure all centroids are fresh
    ws->getBox()->refreshCentroid();

    typedef IMDBox<MDE,nd>* boxPtr;

    // Instrument associated with workspace
    IInstrument_sptr inst = ws->getInstrument();

    // Calculate a threshold below which a box is too diffuse to be considered a peak.
    signal_t thresholdDensity = 0.0;
    thresholdDensity = ws->getBox()->getSignalNormalized() * DensityThresholdFactor;
    g_log.notice() << "Threshold signal density: " << thresholdDensity << std::endl;

    // We will fill this vector with pointers to all the boxes (up to a given depth)
    typename std::vector<boxPtr> boxes;

    // TODO: Don't go to unlimited depth maybe in the future?
    prog->report("Getting Boxes");
    ws->getBox()->getBoxes(boxes, 1000, false); //TODO: Do we want only the leaves?

    // TODO: Here keep only the boxes > e.g. 3 * mean.
    typedef std::pair<double, boxPtr> dens_box;

    // Map that will sort the boxes by increasing density. The key = density; value = box *.
    typename std::multimap<double, boxPtr> sortedBoxes;

    prog->report("Sorting Boxes by Density");
    typename std::vector<boxPtr>::iterator it1;
    typename std::vector<boxPtr>::iterator it1_end = boxes.end();
    for (it1 = boxes.begin(); it1 != it1_end; it1++)
    {
      boxPtr box = *it1;
      double density = box->getSignalNormalized();
      // Skip any boxes with too small a signal density.
      if (density > thresholdDensity)
        sortedBoxes.insert(dens_box(density,box));
    }

    // List of chosen possible peak boxes.
    std::vector<boxPtr> peakBoxes;

    prog->report("Evaluating Strong Boxes");
    int64_t numBoxesFound = 0;
    // Now we go (backwards) through the map
    // e.g. from highest density down to lowest density.
    typename std::multimap<double, boxPtr>::reverse_iterator it2;
    typename std::multimap<double, boxPtr>::reverse_iterator it2_end = sortedBoxes.rend();
    for (it2 = sortedBoxes.rbegin(); it2 != it2_end; it2++)
    {
      signal_t density = it2->first;
      boxPtr box = it2->second;
#ifndef MDBOX_TRACK_CENTROID
      coord_t boxCenter[nd];
      box->calculateCentroid(boxCenter);
#else
      const coord_t * boxCenter = box->getCentroid();
#endif

      // Compare to all boxes already picked.
      bool badBox = false;
      for (typename std::vector<boxPtr>::iterator it3=peakBoxes.begin(); it3 != peakBoxes.end(); it3++)
      {

#ifndef MDBOX_TRACK_CENTROID
        coord_t otherCenter[nd];
        (*it3)->calculateCentroid(otherCenter);
#else
        const coord_t * otherCenter = (*it3)->getCentroid();
#endif

        // Distance between this box and a box we already put in.
        coord_t distSquared = 0.0;
        for (size_t d=0; d<nd; d++)
        {
          coord_t dist = otherCenter[d] - boxCenter[d];
          distSquared += (dist * dist);
        }

        // Reject this box if it is too close to another previously found box.
        if (distSquared < peakRadiusSquared)
        {
          badBox = true;
          break;
        }
      }

      // The box was not rejected for another reason.
      if (!badBox)
      {
        if (numBoxesFound++ > MaxPeaks)
        {
          g_log.notice() << "Number of peaks found exceeded the limit of " << MaxPeaks << ". Stopping peak finding." << std::endl;
          break;
        }

        peakBoxes.push_back(box);
        g_log.information() << "Found box at " << boxCenter[0] << "," << boxCenter[1] << "," << boxCenter[2] << "; Density = " << density << std::endl;
      }
    }

    prog->report("Making PeaksWorkspace");
    peakWS->setInstrument(inst);

    // --- Convert the "boxes" to peaks ----
    for (typename std::vector<boxPtr>::iterator it3=peakBoxes.begin(); it3 != peakBoxes.end(); it3++)
    {
      // The center of the box = Q in the lab frame
      boxPtr box = *it3;
#ifndef MDBOX_TRACK_CENTROID
      coord_t boxCenter[nd];
      box->calculateCentroid(boxCenter);
#else
      const coord_t * boxCenter = box->getCentroid();
#endif

      V3D Q(boxCenter[0], boxCenter[1], boxCenter[2]);

      // Create a peak and add it
      try
      {
        Peak p(inst, Q);
        // TODO: Goniometer matrix and other stuff?

        // Look for a detector
        p.findDetectorUsingQ();

        // The "bin count" used will be the box density.
        p.setBinCount( box->getSignalNormalized() );

        peakWS->addPeak(p);
      }
      catch (std::exception &e)
      {
        g_log.notice() << "Error adding peak at " << Q << " because of '" << e.what() << "'. Peak will be skipped." << std::endl;
      }

    }
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MDEWFindPeaks::exec()
  {
    /// Output peaks workspace, create if needed
    peakWS = getProperty("OutputWorkspace");
    if (!peakWS)
      peakWS = PeaksWorkspace_sptr(new PeaksWorkspace());
    setProperty("OutputWorkspace", peakWS);

    // The MDEventWorkspace as input
    IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");

    // Other parameters
    double PeakDistanceThreshold = getProperty("PeakDistanceThreshold");
    peakRadiusSquared = PeakDistanceThreshold*PeakDistanceThreshold;

    DensityThresholdFactor = getProperty("DensityThresholdFactor");
    MaxPeaks = getProperty("MaxPeaks");

    // TODO: Check that the workspace dimensions are in Q-sample-frame or Q-lab-frame.

    prog = new Progress(this, 0.0, 1.0, 10);

    CALL_MDEVENT_FUNCTION(this->findPeaks, inWS);

    delete prog;
  }



} // namespace Mantid
} // namespace MDEvents

