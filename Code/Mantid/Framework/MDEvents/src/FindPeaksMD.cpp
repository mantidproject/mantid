/*WIKI* 


This algorithm is used to find single-crystal peaks in a multi-dimensional workspace ([[MDEventWorkspace]] or [[MDHistoWorkspace]]).
It looks for high signal density areas, and is based on an algorithm designed by Dennis Mikkelson for ISAW.

The algorithm proceeds in this way:
* Sorts all the boxes in the workspace by decreasing order of signal density (total weighted event sum divided by box volume).
** It will skip any boxes with a density below a threshold. The threshold is <math>TotalSignal / TotalVolume * DensityThresholdFactor</math>.
* The centroid of the strongest box is considered a peak.
* The centroid of the next strongest box is calculated. 
** We look through all the peaks that have already been found. If the box is too close to an existing peak, it is rejected. This distance is PeakDistanceThreshold.
* This is repeated until we find up to MaxPeaks peaks.

Each peak created is placed in the output [[PeaksWorkspace]], which can be a new workspace or replace the old one.

This algorithm works on a [[MDHistoWorkspace]] resulting from the [[BinMD]] algorithm also.
It works in the same way, except that the center of each bin is used since the centroid is not accessible.
It may give better results on [[Workspace2D]]'s that were converted to [[MDWorkspace]]s.

*WIKI*/

#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/FindPeaksMD.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include <map>
#include <vector>
#include "MantidKernel/VMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FindPeaksMD)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FindPeaksMD::FindPeaksMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FindPeaksMD::~FindPeaksMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void FindPeaksMD::initDocs()
  {
    this->setWikiSummary("Find peaks in reciprocal space in a MDEventWorkspace or a MDHistoWorkspace.");
    this->setOptionalMessage("Find peaks in reciprocal space in a MDEventWorkspace or a MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void FindPeaksMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace","",Direction::Input),
        "An input MDEventWorkspace or MDHistoWorkspace with at least 3 dimensions.");

    declareProperty(new PropertyWithValue<double>("PeakDistanceThreshold", 0.1, Direction::Input),
        "Threshold distance for rejecting peaks that are found to be too close from each other.\n"
        "This should be some multiple of the radius of a peak. Default: 0.1."
        );

    declareProperty(new PropertyWithValue<int64_t>("MaxPeaks",500,Direction::Input),
        "Maximum number of peaks to find. Default: 500."
        );

    declareProperty(new PropertyWithValue<double>("DensityThresholdFactor", 10.0, Direction::Input),
        "The overall signal density of the workspace will be multiplied by this factor \n"
        "to get a threshold signal density below which boxes are NOT considered to be peaks. See the help.\n"
        "Default: 10.0"
        );

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output),
        "An output PeaksWorkspace with the peaks' found positions.");

    declareProperty("AppendPeaks", false,
        "If checked, then append the peaks in the output workspace if it exists. \n"
        "If unchecked, the output workspace is replaced (Default)."  );

  }

  //----------------------------------------------------------------------------------------------
  /** Extract needed data from the workspace's experiment info */
  void FindPeaksMD::readExperimentInfo(ExperimentInfo_sptr ei, IMDWorkspace_sptr ws)
  {
    // Instrument associated with workspace
    inst = ei->getInstrument();
    // Find the run number
    runNumber = ei->getRunNumber();

    // Check that the workspace dimensions are in Q-sample-frame or Q-lab-frame.
    std::string dim0 = ws->getDimension(0)->getName();
    if (dim0 == "H")
    {
      dimType = HKL;
      throw std::runtime_error("Cannot find peaks in a workspace that is already in HKL space.");
    }
    else if (dim0 == "Q_lab_x")
    {
      dimType = QLAB;
    }
    else if (dim0 == "Q_sample_x")
      dimType = QSAMPLE;
    else
      throw std::runtime_error("Unexpected dimensions: need either Q_lab_x or Q_sample_x.");

    // Find the goniometer rotation matrix
    goniometer = Mantid::Kernel::Matrix<double>(3,3, true); // Default IDENTITY matrix
    try
    {
      goniometer = ei->mutableRun().getGoniometerMatrix();
    }
    catch (std::exception & e)
    {
      g_log.warning() << "Error finding goniometer matrix. It will not be set in the peaks found." << std::endl;
      g_log.warning() << e.what() << std::endl;
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Create and add a Peak to the output workspace
   *
   * @param Q :: Q_lab or Q_sample, depending on workspace
   * @param binCount :: bin count to give to the peak.
   */
  void FindPeaksMD::addPeak(V3D Q, double binCount)
  {
    // Create a peak and add it
    // Empty starting peak.
    Peak p;
    try
    {
      if (dimType == QLAB)
      {
        // Build using the Q-lab-frame constructor
        p = Peak(inst, Q);
        // Save gonio matrix for later
        p.setGoniometerMatrix(goniometer);
      }
      else if (dimType == QSAMPLE)
      {
        // Build using the Q-sample-frame constructor
        p = Peak(inst, Q, goniometer);
      }
    }
    catch (std::exception &e)
    {
      g_log.notice() << "Error creating peak at " << Q << " because of '" << e.what() << "'. Peak will be skipped." << std::endl;
      return;
    }

    try
    { // Look for a detector
      p.findDetector();
    }
    catch (...)
    { /* Ignore errors in ray-tracer */ }

    p.setBinCount( binCount );

    // Save the run number found before.
    p.setRunNumber(runNumber);

    peakWS->addPeak(p);
  }


  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param ws ::  MDEventWorkspace to integrate
   */
  template<typename MDE, size_t nd>
  void FindPeaksMD::findPeaks(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    if (nd < 3)
      throw std::invalid_argument("Workspace must have at least 3 dimensions.");

    progress(0.01, "Refreshing Centroids");

    // TODO: This might be slow, progress report?
    // Make sure all centroids are fresh
    ws->getBox()->refreshCentroid();

    typedef IMDBox<MDE,nd>* boxPtr;

    if (ws->getNumExperimentInfo() == 0)
      throw std::runtime_error("No instrument was found in the MDEventWorkspace. Cannot find peaks.");
    // TODO: Do we need to pick a different instrument info?
    ExperimentInfo_sptr ei = ws->getExperimentInfo(0);
    this->readExperimentInfo(ei, boost::dynamic_pointer_cast<IMDWorkspace>(ws));
    // Copy the instrument, sample, run to the peaks workspace.
    peakWS->copyExperimentInfoFrom(ei.get());

    /// Arbitrary scaling factor for density to make more manageable numbers, especially for older file formats.
    signal_t densityScalingFactor = 1e-6;

    // Calculate a threshold below which a box is too diffuse to be considered a peak.
    signal_t thresholdDensity = 0.0;
    thresholdDensity = ws->getBox()->getSignalNormalized() * DensityThresholdFactor * densityScalingFactor;
    if ((thresholdDensity != thresholdDensity) || (thresholdDensity == std::numeric_limits<double>::infinity())
        || (thresholdDensity == -std::numeric_limits<double>::infinity()))
    {
      g_log.warning() << "Infinite or NaN overall density found. Your input data may be invalid. Using a 0 threshold instead." << std::endl;
      thresholdDensity = 0;
    }
    g_log.notice() << "Threshold signal density: " << thresholdDensity << std::endl;


    // We will fill this vector with pointers to all the boxes (up to a given depth)
    typename std::vector<boxPtr> boxes;

    // Get all the MDboxes
    progress(0.10, "Getting Boxes");
    ws->getBox()->getBoxes(boxes, 1000, true);


    // This pair is the <density, ptr to the box>
    typedef std::pair<double, boxPtr> dens_box;

    // Map that will sort the boxes by increasing density. The key = density; value = box *.
    typename std::multimap<double, boxPtr> sortedBoxes;

    // --------------- Sort and Filter by Density -----------------------------
    progress(0.20, "Sorting Boxes by Density");
    typename std::vector<boxPtr>::iterator it1;
    typename std::vector<boxPtr>::iterator it1_end = boxes.end();
    for (it1 = boxes.begin(); it1 != it1_end; it1++)
    {
      boxPtr box = *it1;
      double density = box->getSignalNormalized() * densityScalingFactor;
      // Skip any boxes with too small a signal density.
      if (density > thresholdDensity)
        sortedBoxes.insert(dens_box(density,box));
    }

    // --------------- Find Peak Boxes -----------------------------
    // List of chosen possible peak boxes.
    std::vector<boxPtr> peakBoxes;

    prog = new Progress(this, 0.30, 0.95, MaxPeaks);

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
        if (numBoxesFound++ >= MaxPeaks)
        {
          g_log.notice() << "Number of peaks found exceeded the limit of " << MaxPeaks << ". Stopping peak finding." << std::endl;
          break;
        }

        peakBoxes.push_back(box);
        g_log.debug() << "Found box at ";
        for (size_t d=0; d<nd; d++)
          g_log.debug() << (d>0?",":"") << boxCenter[d];
        g_log.debug() << "; Density = " << density << std::endl;
        // Report progres for each box found.
        prog->report("Finding Peaks");
      }
    }

    prog->resetNumSteps(numBoxesFound, 0.95, 1.0);

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

      // Q of the centroid of the box
      V3D Q(boxCenter[0], boxCenter[1], boxCenter[2]);

      // The "bin count" used will be the box density.
      double binCount = box->getSignalNormalized() * densityScalingFactor;

      // Create the peak
      addPeak(Q, binCount);

      // Report progres for each box found.
      prog->report("Adding Peaks");

    } // for each box found

  }




  //----------------------------------------------------------------------------------------------
  /** Find peaks in the given MDHistoWorkspace
   *
   * @param ws :: MDHistoWorkspace
   */
  void FindPeaksMD::findPeaksHisto(Mantid::MDEvents::MDHistoWorkspace_sptr ws)
  {
    size_t nd = ws->getNumDims();
    if (nd < 3)
      throw std::invalid_argument("Workspace must have at least 3 dimensions.");

    if (ws->getNumExperimentInfo() == 0)
      throw std::runtime_error("No instrument was found in the workspace. Cannot find peaks.");
    ExperimentInfo_sptr ei = ws->getExperimentInfo(0);
    this->readExperimentInfo(ei, boost::dynamic_pointer_cast<IMDWorkspace>(ws));

    // Copy the instrument, sample, run to the peaks workspace.
    peakWS->copyExperimentInfoFrom(ei.get());

    // This pair is the <density, box index>
    typedef std::pair<double, size_t> dens_box;

    // Map that will sort the boxes by increasing density. The key = density; value = box index.
    std::multimap<double, size_t> sortedBoxes;

    /// Arbitrary scaling factor for density to make more manageable numbers, especially for older file formats.
    signal_t densityScalingFactor = 1e-6;

    size_t numBoxes = ws->getNPoints();

    // --------- Count the overall signal density -----------------------------
    progress(0.10, "Counting Total Signal");
    double totalSignal = 0;
    for (size_t i=0; i<numBoxes; i++)
      totalSignal += ws->getSignalAt(i);
    // Calculate the threshold density
    double thresholdDensity = (totalSignal * ws->getInverseVolume() / double(numBoxes))
                              * DensityThresholdFactor * densityScalingFactor;
    if ((thresholdDensity != thresholdDensity) || (thresholdDensity == std::numeric_limits<double>::infinity())
        || (thresholdDensity == -std::numeric_limits<double>::infinity()))
    {
      g_log.warning() << "Infinite or NaN overall density found. Your input data may be invalid. Using a 0 threshold instead." << std::endl;
      thresholdDensity = 0;
    }
    g_log.notice() << "Threshold signal density: " << thresholdDensity << std::endl;

    // -------------- Sort and Filter by Density -----------------------------
    progress(0.20, "Sorting Boxes by Density");
    for (size_t i=0; i<numBoxes; i++)
    {
      double density = ws->getSignalNormalizedAt(i) * densityScalingFactor;
      // Skip any boxes with too small a signal density.
      if (density > thresholdDensity)
        sortedBoxes.insert(dens_box(density,i));
    }


    // --------------- Find Peak Boxes -----------------------------
    // List of chosen possible peak boxes.
    std::vector<size_t> peakBoxes;

    prog = new Progress(this, 0.30, 0.95, MaxPeaks);

    int64_t numBoxesFound = 0;
    // Now we go (backwards) through the map
    // e.g. from highest density down to lowest density.
    std::multimap<double, size_t>::reverse_iterator it2;
    std::multimap<double, size_t>::reverse_iterator it2_end = sortedBoxes.rend();
    for (it2 = sortedBoxes.rbegin(); it2 != it2_end; ++it2)
    {
      signal_t density = it2->first;
      size_t index = it2->second;
      // Get the center of the box
      VMD boxCenter = ws->getCenter(index);

      // Compare to all boxes already picked.
      bool badBox = false;
      for (std::vector<size_t>::iterator it3=peakBoxes.begin(); it3 != peakBoxes.end(); ++it3)
      {
        VMD otherCenter = ws->getCenter(*it3);

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
        if (numBoxesFound++ >= MaxPeaks)
        {
          g_log.notice() << "Number of peaks found exceeded the limit of " << MaxPeaks << ". Stopping peak finding." << std::endl;
          break;
        }

        peakBoxes.push_back(index);
        g_log.debug() << "Found box at index " << index;
        g_log.debug() << "; Density = " << density << std::endl;
        // Report progres for each box found.
        prog->report("Finding Peaks");
      }
    }
    // --- Convert the "boxes" to peaks ----
    for (std::vector<size_t>::iterator it3=peakBoxes.begin(); it3 != peakBoxes.end(); ++it3)
    {
      size_t index = *it3;
      // The center of the box = Q in the lab frame
      VMD boxCenter = ws->getCenter(index);

      // Q of the centroid of the box
      V3D Q(boxCenter[0], boxCenter[1], boxCenter[2]);

      // The "bin count" used will be the box density.
      double binCount = ws->getSignalNormalizedAt(index) * densityScalingFactor;

      // Create the peak
      addPeak(Q, binCount);

      // Report progres for each box found.
      prog->report("Adding Peaks");

    } // for each box found

  }




  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void FindPeaksMD::exec()
  {
    bool AppendPeaks = getProperty("AppendPeaks");

    // Output peaks workspace, create if needed
    peakWS = getProperty("OutputWorkspace");
    if (!peakWS || !AppendPeaks)
      peakWS = PeaksWorkspace_sptr(new PeaksWorkspace());

    // The MDEventWorkspace as input
    IMDWorkspace_sptr inWS = getProperty("InputWorkspace");
    MDHistoWorkspace_sptr inMDHW = boost::dynamic_pointer_cast<MDHistoWorkspace>(inWS);
    IMDEventWorkspace_sptr inMDEW = boost::dynamic_pointer_cast<IMDEventWorkspace>(inWS);

    // Other parameters
    double PeakDistanceThreshold = getProperty("PeakDistanceThreshold");
    peakRadiusSquared = static_cast<coord_t>(PeakDistanceThreshold*PeakDistanceThreshold);

    DensityThresholdFactor = getProperty("DensityThresholdFactor");
    MaxPeaks = getProperty("MaxPeaks");

    // Execute the proper algo based on the type of workspace
    if (inMDHW)
    {
      this->findPeaksHisto(inMDHW);
    }
    else if (inMDEW)
    {
      CALL_MDEVENT_FUNCTION3(this->findPeaks, inMDEW);
    }
    else
    {
      throw std::runtime_error("This algorithm can only find peaks on a MDHistoWorkspace or a MDEventWorkspace; it does not work on a regular MatrixWorkspace.");
    }

    delete prog;

    // Do a sort by bank name and then descending bin count (intensity)
    std::vector< std::pair<std::string, bool> > criteria;
    criteria.push_back( std::pair<std::string, bool>("BankName", true) );
    criteria.push_back( std::pair<std::string, bool>("bincount", false) );
    peakWS->sort(criteria);

    // Save the output
    setProperty("OutputWorkspace", peakWS);

  }



} // namespace Mantid
} // namespace MDEvents

