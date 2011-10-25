/*WIKI*
Algorithm that can take a slice out of an original MDEventWorkspace while preserving all the events contained therein.
*WIKI*/

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/SliceMD.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadPool.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::MDImplicitFunction;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SliceMD)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SliceMD::SliceMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SliceMD::~SliceMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SliceMD::initDocs()
  {
    this->setWikiSummary("Make a MDEventWorkspace containing the events in a slice of an input MDEventWorkspace.");
    this->setOptionalMessage("Make a MDEventWorkspace containing the events in a slice of an input MDEventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SliceMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");

    // Properties for specifying the slice to perform.
    this->initSlicingProps();

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Copy the extra data (not signal, error or coordinates) from one event to another
   * with different numbers of dimensions
   *
   * @param srcEvent :: the source event, being copied
   * @param newEvent :: the destination event
   */
  template<size_t nd, size_t ond>
  inline void copyEvent(const MDLeanEvent<nd> & /*srcEvent*/, MDLeanEvent<ond> & /*newEvent*/)
  {
    // Nothing extra copy - this is no-op
  }

  //----------------------------------------------------------------------------------------------
  /** Copy the extra data (not signal, error or coordinates) from one event to another
   * with different numbers of dimensions
   *
   * @param srcEvent :: the source event, being copied
   * @param newEvent :: the destination event
   */
  template<size_t nd, size_t ond>
  inline void copyEvent(const MDEvent<nd> & srcEvent, MDEvent<ond> & newEvent)
  {
    newEvent.setDetectorId(srcEvent.getDetectorID());
    newEvent.setRunIndex(srcEvent.getRunIndex());
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the slice from nd input dimensions to ond output dimensions
   *
   * @param ws :: input workspace with nd dimensions
   * @tparam OMDE :: MDEvent type for the OUTPUT workspace
   * @tparam ond :: number of dimensions in the OUTPUT workspace
   */
  template<typename MDE, size_t nd, typename OMDE, size_t ond>
  void SliceMD::slice(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // Create the ouput workspace
    typename MDEventWorkspace<OMDE, ond>::sptr outWS(new MDEventWorkspace<OMDE, ond>());
    for (size_t od=0; od < binDimensions.size(); od++)
      outWS->addDimension(binDimensions[od]);
    outWS->initialize();
    // Copy settings from the original box controller
    BoxController_sptr bc = ws->getBoxController();
    BoxController_sptr obc = outWS->getBoxController();
    // Use the "number of bins" as the "split into" parameter
    for (size_t od=0; od < binDimensions.size(); od++)
      obc->setSplitInto(od, binDimensions[od]->getNBins());
    obc->setSplitThreshold(bc->getSplitThreshold());
    obc->setMaxDepth(bc->getMaxDepth());
    obc->resetNumBoxes();
    // Perform the first box splitting
    outWS->splitBox();

    // Function defining which events (in the input dimensions) to place in the output
    MDImplicitFunction * function = this->getImplicitFunctionForChunk(NULL, NULL);

    std::vector<IMDBox<MDE,nd>*> boxes;
    // Leaf-only; no depth limit; with the implicit function passed to it.
    ws->getBox()->getBoxes(boxes, 1000, true, function);
    // Sort boxes by file position IF file backed. This reduces seeking time, hopefully.
    if (bc->isFileBacked())
      IMDBox<MDE, nd>::sortBoxesByFilePos(boxes);

    Progress * prog = new Progress(this, 0.0, 1.0, boxes.size());

    // The root of the output workspace
    IMDBox<OMDE,ond>* outRootBox = outWS->getBox();

    uint64_t totalAdded = 0;
    uint64_t numSinceSplit = 0;

    // Go through every box for this chunk.
    //PARALLEL_FOR_IF( !bc->isFileBacked() )
    for (int i=0; i<int(boxes.size()); i++)
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
      // Perform the binning in this separate method.
      if (box)
      {
        // An array to hold the rotated/transformed coordinates
        coord_t outCenter[ond];

        const std::vector<MDE> & events = box->getConstEvents();
        typename std::vector<MDE>::const_iterator it = events.begin();
        typename std::vector<MDE>::const_iterator it_end = events.end();
        for (; it != it_end; it++)
        {
          // Cache the center of the event (again for speed)
          const coord_t * inCenter = it->getCenter();

          if (function->isPointContained(inCenter))
          {
            // Now transform to the output dimensions
            m_transformFromOriginal->apply(inCenter, outCenter);

            // Create the event
            OMDE newEvent(it->getSignal(), it->getErrorSquared(), outCenter);
            // Copy extra data, if any
            copyEvent(*it, newEvent);
            // Add it to the workspace
            outRootBox->addEvent( newEvent );

            numSinceSplit++;
          }
        }

        // Every 20 million events, or at the last box: do splitting
        if (numSinceSplit > 20000000 || (i == int(boxes.size()-1)))
        {
          // This splits up all the boxes according to split thresholds and sizes.
          Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
          ThreadPool tp(ts);
          outWS->splitAllIfNeeded(ts);
          tp.joinAll();
          // Accumulate stats
          totalAdded += numSinceSplit;
          numSinceSplit = 0;
        }
      }

      // Progress reporting
      prog->report();

    }// for each box in the vector

    // Refresh all cache.
    outWS->refreshCache();

    g_log.notice() << totalAdded << " " << OMDE::getTypeName() << "'s added to the output workspace." << std::endl;

    this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(outWS));
    delete prog;
  }


  //----------------------------------------------------------------------------------------------
  /// Helper method
  template<typename MDE, size_t nd>
  void SliceMD::doExec(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // Templated method needs to call another templated method depending on the # of output dimensions.
    if (MDE::getTypeName() == "MDLeanEvent")
    {
      if (outD==1)      this->slice<MDE,nd,MDLeanEvent<1>,1>(ws);
      else if (outD==2) this->slice<MDE,nd,MDLeanEvent<2>,2>(ws);
      else if (outD==3) this->slice<MDE,nd,MDLeanEvent<3>,3>(ws);
      else if (outD==4) this->slice<MDE,nd,MDLeanEvent<4>,4>(ws);
      else
        throw std::runtime_error("Number of output dimensions > 4 or < 1. This is not currently handled.");
    }
    else if (MDE::getTypeName() == "MDEvent")
    {
      if (outD==1)      this->slice<MDE,nd,MDEvent<1>,1>(ws);
      else if (outD==2) this->slice<MDE,nd,MDEvent<2>,2>(ws);
      else if (outD==3) this->slice<MDE,nd,MDEvent<3>,3>(ws);
      else if (outD==4) this->slice<MDE,nd,MDEvent<4>,4>(ws);
      else
        throw std::runtime_error("Number of output dimensions > 4 or < 1. This is not currently handled.");
    }
    else
      throw std::runtime_error("Unexpected MDEvent type '" + MDE::getTypeName() + "'. This is not currently handled.");
  }



  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SliceMD::exec()
  {
    // Input MDEventWorkspace
    in_ws = getProperty("InputWorkspace");

    // Run through the properties to create the transform you need
    createTransform();

    CALL_MDEVENT_FUNCTION(this->doExec, in_ws);
  }



} // namespace Mantid
} // namespace MDEvents

