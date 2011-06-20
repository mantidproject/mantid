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

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output),
        "An output PeaksWorkspace with the peaks' found positions.");
  }


  //----------------------------------------------------------------------------------------------
  /** A possible peak: a sub-class of Peak with extra information
   * for finding/classifying peaks.
   */
  template<typename MDE, size_t nd>
  class PossiblePeak // : public Peak
  {
  public:

    coord_t center;
    signal_t density;

    /** Constructor
     *
     * @param box :: IMDBox that is the starting point of the peak location.
     * @return
     */
    PossiblePeak(IMDBox<MDE,nd>* /*box*/)
    {
    }
  };


  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param ws ::  MDEventWorkspace to integrate
   */
  template<typename MDE, size_t nd>
  void MDEWFindPeaks::findPeaks(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    typedef IMDBox<MDE,nd>* boxPtr;
    //IInstrument_sptr inst;// = ws->getInstrument();

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

    // Now we go (backwards) through the map
    // e.g. from highest density down to lowest density.
    typename std::multimap<double, boxPtr>::reverse_iterator it2;
    typename std::multimap<double, boxPtr>::reverse_iterator it2_end = sortedBoxes.rend();
    for (it2 = sortedBoxes.rbegin(); it2 != it2_end; it2++)
    {
      boxPtr box = it2->second;

      PossiblePeak<MDE,nd> pp(box);
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

    // TODO: Check that the workspace dimensions are in Q-sample-frame.

    CALL_MDEVENT_FUNCTION(this->findPeaks, inWS);
  }



} // namespace Mantid
} // namespace MDEvents

