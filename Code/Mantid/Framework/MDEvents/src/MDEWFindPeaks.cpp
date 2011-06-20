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
        ""
        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MDEWFindPeaks::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");

    declareProperty(new PropertyWithValue<double>("PeakDistanceThreshold",1.0,Direction::Input),
        "Threshold distance for rejecting peaks that are found to be too close from each other.\n"
        "This should be some multiple of the radius of a peak."
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
    // TODO: This might be slow, progress report?
    // Make sure all centroids are fresh
    ws->getBox()->refreshCentroid();

    typedef IMDBox<MDE,nd>* boxPtr;

    // Instrument associated with workspace
    IInstrument_sptr inst = ws->getInstrument();

    // Calculate a threshold below which a box is too diffuse to be considered a peak.
    signal_t thresholdDensity = 0.0;
    thresholdDensity = ws->getBox()->getSignalNormalized() * DensityThresholdFactor;
    g_log.information() << "Threshold signal density: " << thresholdDensity << std::endl;

    // We will fill this vector with pointers to all the boxes (up to a given depth)
    typename std::vector<boxPtr> boxes;
    // TODO: Don't go to unlimited depth maybe in the future?
    ws->getBox()->getBoxes(boxes, 1000);

    // TODO: Here keep only the boxes > e.g. 3 * mean.
    typedef std::pair<double, boxPtr> dens_box;

    // Map that will sort the boxes by increasing density. The key = density; value = box *.
    typename std::multimap<double, boxPtr> sortedBoxes;

    typename std::vector<boxPtr>::iterator it1;
    typename std::vector<boxPtr>::iterator it1_end = boxes.end();
    for (it1 = boxes.begin(); it1 != it1_end; it1++)
    {
      boxPtr box = *it1;
      double density = box->getSignalNormalized();
      sortedBoxes.insert(dens_box(density,box));
    }

    // List of chosen possible peak boxes.
    std::vector<boxPtr> peakBoxes;

    // Now we go (backwards) through the map
    // e.g. from highest density down to lowest density.
    typename std::multimap<double, boxPtr>::reverse_iterator it2;
    typename std::multimap<double, boxPtr>::reverse_iterator it2_end = sortedBoxes.rend();
    for (it2 = sortedBoxes.rbegin(); it2 != it2_end; it2++)
    {
      // Stop as soon as you hight the first box with too small a signal density.
      signal_t density = it2->first;
      if (density <= thresholdDensity)
        break;

      boxPtr box = it2->second;
      const coord_t * boxCenter = box->getCentroid();

      // Compare to all boxes already picked.
      bool badBox = false;
      for (typename std::vector<boxPtr>::iterator it3=peakBoxes.begin(); it3 != peakBoxes.end(); it3++)
      {
        const coord_t * otherCenter = (*it3)->getCentroid();
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
        peakBoxes.push_back(box);
        g_log.information() << "Found box at " << boxCenter[0] << "," << boxCenter[1] << "," << boxCenter[2] << "; Density = " << density << std::endl;
      }
    }

    // --- Convert the "boxes" to peaks ----
    for (typename std::vector<boxPtr>::iterator it3=peakBoxes.begin(); it3 != peakBoxes.end(); it3++)
    {
      // The center of the box = Q in the lab frame
      boxPtr box = *it3;
      V3D Q(box->getCentroid(0), box->getCentroid(1), box->getCentroid(2));

      // Create a peak and add it
      Peak p(inst, Q);
      // The "bin count" used will be the box density.
      p.setBinCount( box->getSignalNormalized() );
      try
      {
        // TODO: Goniometer matrix and other stuff?
        peakWS->addPeak(p);
      }
      catch (std::runtime_error &e)
      {
        g_log.warning() << "Error adding peak at " << Q << " because of '" << e.what() << "'. Peak will be skipped." << std::endl;
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

    // TODO: Check that the workspace dimensions are in Q-sample-frame or Q-lab-frame.

    CALL_MDEVENT_FUNCTION(this->findPeaks, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

