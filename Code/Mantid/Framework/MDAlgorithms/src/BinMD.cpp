#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/CoordTransformAffineParser.h"
#include "MantidMDEvents/CoordTransformAligned.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/BinMD.h"
#include <boost/algorithm/string.hpp>
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidMDEvents/CoordTransformAffine.h"

using Mantid::Kernel::CPUTimer;
using Mantid::Kernel::EnabledWhenProperty;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(BinMD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
BinMD::BinMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
BinMD::~BinMD() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void BinMD::init() {
  declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace", "",
                                                      Direction::Input),
                  "An input MDWorkspace.");

  // Properties for specifying the slice to perform.
  this->initSlicingProps();

  // --------------- Processing methods and options
  // ---------------------------------------
  std::string grp = "Methods";
  declareProperty(new PropertyWithValue<std::string>("ImplicitFunctionXML", "",
                                                     Direction::Input),
                  "XML string describing the implicit function determining "
                  "which bins to use.");
  setPropertyGroup("ImplicitFunctionXML", grp);

  declareProperty(
      new PropertyWithValue<bool>("IterateEvents", true, Direction::Input),
      "Alternative binning method where you iterate through every event, "
      "placing them in the proper bin.\n"
      "This may be faster for workspaces with few events and lots of output "
      "bins.");
  setPropertyGroup("IterateEvents", grp);

  declareProperty(
      new PropertyWithValue<bool>("Parallel", false, Direction::Input),
      "Temporary parameter: true to run in parallel. This is ignored for "
      "file-backed workspaces, where running in parallel makes things slower "
      "due to disk thrashing.");
  setPropertyGroup("Parallel", grp);

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "A name for the output MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Bin the contents of a MDBox
 *
 * @param box :: pointer to the MDBox to bin
 * @param chunkMin :: the minimum index in each dimension to consider "valid"
 *(inclusive)
 * @param chunkMax :: the maximum index in each dimension to consider "valid"
 *(exclusive)
 */
template <typename MDE, size_t nd>
inline void BinMD::binMDBox(MDBox<MDE, nd> *box, const size_t *const chunkMin,
                            const size_t *const chunkMax) {
  // An array to hold the rotated/transformed coordinates
  coord_t *outCenter = new coord_t[m_outD];

  // Evaluate whether the entire box is in the same bin
  if (box->getNPoints() > (1 << nd) * 2) {
    // There is a check that the number of events is enough for it to make sense
    // to do all this processing.
    size_t numVertexes = 0;
    coord_t *vertexes = box->getVertexesArray(numVertexes);

    // All vertexes have to be within THE SAME BIN = have the same linear index.
    size_t lastLinearIndex = 0;
    bool badOne = false;

    for (size_t i = 0; i < numVertexes; i++) {
      // Cache the center of the event (again for speed)
      const coord_t *inCenter = vertexes + i * nd;

      // Now transform to the output dimensions
      m_transform->apply(inCenter, outCenter);
      // std::cout << "Input coord " << VMD(nd,inCenter) << " transformed to "
      // <<  VMD(nd,outCenter) << std::endl;

      // To build up the linear index
      size_t linearIndex = 0;
      // To mark VERTEXES outside range
      badOne = false;

      /// Loop through the dimensions on which we bin
      for (size_t bd = 0; bd < m_outD; bd++) {
        // What is the bin index in that dimension
        coord_t x = outCenter[bd];
        size_t ix = size_t(x);
        // Within range (for this chunk)?
        if ((x >= 0) && (ix >= chunkMin[bd]) && (ix < chunkMax[bd])) {
          // Build up the linear index
          linearIndex += indexMultiplier[bd] * ix;
        } else {
          // Outside the range
          badOne = true;
          break;
        }
      } // (for each dim in MDHisto)

      // Is the vertex at the same place as the last one?
      if (!badOne) {
        if ((i > 0) && (linearIndex != lastLinearIndex)) {
          // Change of index
          badOne = true;
          break;
        }
        lastLinearIndex = linearIndex;
      }

      // Was the vertex completely outside the range?
      if (badOne)
        break;
    } // (for each vertex)

    delete[] vertexes;

    if (!badOne) {
      // Yes, the entire box is within a single bin
      //        std::cout << "Box at " << box->getExtentsStr() << " is within a
      //        single bin.\n";
      // Add the CACHED signal from the entire box
      signals[lastLinearIndex] += box->getSignal();
      errors[lastLinearIndex] += box->getErrorSquared();
      // TODO: If MDEvents get a weight, this would need to get the summed
      // weight.
      numEvents[lastLinearIndex] += static_cast<signal_t>(box->getNPoints());

      // And don't bother looking at each event. This may save lots of time
      // loading from disk.
      delete[] outCenter;
      return;
    }
  }

  // If you get here, you could not determine that the entire box was in the
  // same bin.
  // So you need to iterate through events.

  const std::vector<MDE> &events = box->getConstEvents();
  typename std::vector<MDE>::const_iterator it = events.begin();
  typename std::vector<MDE>::const_iterator it_end = events.end();
  for (; it != it_end; it++) {
    // Cache the center of the event (again for speed)
    const coord_t *inCenter = it->getCenter();

    // Now transform to the output dimensions
    m_transform->apply(inCenter, outCenter);

    // To build up the linear index
    size_t linearIndex = 0;
    // To mark events outside range
    bool badOne = false;

    /// Loop through the dimensions on which we bin
    for (size_t bd = 0; bd < m_outD; bd++) {
      // What is the bin index in that dimension
      coord_t x = outCenter[bd];
      size_t ix = size_t(x);
      // Within range (for this chunk)?
      if ((x >= 0) && (ix >= chunkMin[bd]) && (ix < chunkMax[bd])) {
        // Build up the linear index
        linearIndex += indexMultiplier[bd] * ix;
      } else {
        // Outside the range
        badOne = true;
        break;
      }
    } // (for each dim in MDHisto)

    if (!badOne) {
      // Sum the signals as doubles to preserve precision
      signals[linearIndex] += static_cast<signal_t>(it->getSignal());
      errors[linearIndex] += static_cast<signal_t>(it->getErrorSquared());
      // TODO: If MDEvents get a weight, this would need to get the summed
      // weight.
      numEvents[linearIndex] += 1.0;
    }
  }
  // Done with the events list
  box->releaseEvents();

  delete[] outCenter;
}

//----------------------------------------------------------------------------------------------
/** Perform binning by iterating through every event and placing them in the
 *output workspace
 *
 * @param ws :: MDEventWorkspace of the given type.
 */
template <typename MDE, size_t nd>
void BinMD::binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  BoxController_sptr bc = ws->getBoxController();
  // store exisiting write buffer size for the future
  // uint64_t writeBufSize =bc->getDiskBuffer().getWriteBufferSize();
  // and disable write buffer (if any) for input MD Events for this algorithm
  // purposes;
  // bc->setCacheParameters(1,0);

  // Cache some data to speed up accessing them a bit
  indexMultiplier = new size_t[m_outD];
  for (size_t d = 0; d < m_outD; d++) {
    if (d > 0)
      indexMultiplier[d] = outWS->getIndexMultiplier()[d - 1];
    else
      indexMultiplier[d] = 1;
  }
  signals = outWS->getSignalArray();
  errors = outWS->getErrorSquaredArray();
  numEvents = outWS->getNumEventsArray();

  // Start with signal/error/numEvents at 0.0
  outWS->setTo(0.0, 0.0, 0.0);

  // The dimension (in the output workspace) along which we chunk for parallel
  // processing
  // TODO: Find the smartest dimension to chunk against
  size_t chunkDimension = 0;

  // How many bins (in that dimension) per chunk.
  // Try to split it so each core will get 2 tasks:
  int chunkNumBins = int(m_binDimensions[chunkDimension]->getNBins() /
                         (PARALLEL_GET_MAX_THREADS * 2));
  if (chunkNumBins < 1)
    chunkNumBins = 1;

  // Do we actually do it in parallel?
  bool doParallel = getProperty("Parallel");
  // Not if file-backed!
  if (bc->isFileBacked())
    doParallel = false;
  if (!doParallel)
    chunkNumBins = int(m_binDimensions[chunkDimension]->getNBins());

  // Total number of steps
  size_t progNumSteps = 0;
  if (prog)
    prog->setNotifyStep(0.1);
  if (prog)
    prog->resetNumSteps(100, 0.00, 1.0);

  // Run the chunks in parallel. There is no overlap in the output workspace so
  // it is
  // thread safe to write to it..
  // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for schedule(dynamic,1) if (doParallel) )
    for (int chunk = 0;
         chunk < int(m_binDimensions[chunkDimension]->getNBins());
         chunk += chunkNumBins) {
      PARALLEL_START_INTERUPT_REGION
      // Region of interest for this chunk.
      std::vector<size_t> chunkMin(m_outD);
      std::vector<size_t> chunkMax(m_outD);
      for (size_t bd = 0; bd < m_outD; bd++) {
        // Same limits in the other dimensions
        chunkMin[bd] = 0;
        chunkMax[bd] = m_binDimensions[bd]->getNBins();
      }
      // Parcel out a chunk in that single dimension dimension
      chunkMin[chunkDimension] = size_t(chunk);
      if (size_t(chunk + chunkNumBins) >
          m_binDimensions[chunkDimension]->getNBins())
        chunkMax[chunkDimension] = m_binDimensions[chunkDimension]->getNBins();
      else
        chunkMax[chunkDimension] = size_t(chunk + chunkNumBins);

      // Build an implicit function (it needs to be in the space of the
      // MDEventWorkspace)
      MDImplicitFunction *function =
          this->getImplicitFunctionForChunk(chunkMin.data(), chunkMax.data());

      // Use getBoxes() to get an array with a pointer to each box
      std::vector<API::IMDNode *> boxes;
      // Leaf-only; no depth limit; with the implicit function passed to it.
      ws->getBox()->getBoxes(boxes, 1000, true, function);

      // Sort boxes by file position IF file backed. This reduces seeking time,
      // hopefully.
      if (bc->isFileBacked())
        API::IMDNode::sortObjByID(boxes);

      // For progress reporting, the # of boxes
      if (prog) {
        PARALLEL_CRITICAL(BinMD_progress) {
          g_log.debug() << "Chunk " << chunk << ": found " << boxes.size()
                        << " boxes within the implicit function." << std::endl;
          progNumSteps += boxes.size();
          prog->setNumSteps(progNumSteps);
        }
      }

      // Go through every box for this chunk.
      for (size_t i = 0; i < boxes.size(); i++) {
        MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
        // Perform the binning in this separate method.
        if (box)
          this->binMDBox(box, chunkMin.data(), chunkMax.data());

        // Progress reporting
        if (prog)
          prog->report();
        // For early cancelling of the loop
        if (this->m_cancel)
          break;
      } // for each box in the vector
      PARALLEL_END_INTERUPT_REGION
    } // for each chunk in parallel
    PARALLEL_CHECK_INTERUPT_REGION

    // Now the implicit function
    if (implicitFunction) {
      prog->report("Applying implicit function.");
      signal_t nan = std::numeric_limits<signal_t>::quiet_NaN();
      outWS->applyImplicitFunction(implicitFunction, nan, nan);
    }

    // return the size of the input workspace write buffer to its initial value
    // bc->setCacheParameters(sizeof(MDE),writeBufSize);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void BinMD::exec() {
  // Input MDEventWorkspace/MDHistoWorkspace
  m_inWS = getProperty("InputWorkspace");
  // Look at properties, create either axis-aligned or general transform.
  // This (can) change m_inWS
  this->createTransform();

  // De serialize the implicit function
  std::string ImplicitFunctionXML = getPropertyValue("ImplicitFunctionXML");
  implicitFunction = NULL;
  if (!ImplicitFunctionXML.empty())
    implicitFunction =
        Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(
            ImplicitFunctionXML);

  prog = new Progress(
      this, 0, 1.0,
      1); // This gets deleted by the thread pool; don't delete it in here.

  // Create the dense histogram. This allocates the memory
  outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(m_binDimensions));

  // Saves the geometry transformation from original to binned in the workspace
  outWS->setTransformFromOriginal(this->m_transformFromOriginal, 0);
  outWS->setTransformToOriginal(this->m_transformToOriginal, 0);
  for (size_t i = 0; i < m_bases.size(); i++)
    outWS->setBasisVector(i, m_bases[i]);
  outWS->setOrigin(this->m_translation);
  outWS->setOriginalWorkspace(m_inWS, 0);

  // And the intermediate WS one too, if any
  if (m_intermediateWS) {
    outWS->setOriginalWorkspace(m_intermediateWS, 1);
    outWS->setTransformFromOriginal(m_transformFromIntermediate, 1);
    outWS->setTransformToOriginal(m_transformToIntermediate, 1);
  }

  // Wrapper to cast to MDEventWorkspace then call the function
  bool IterateEvents = getProperty("IterateEvents");
  if (!IterateEvents) {
    g_log.warning() << "IterateEvents=False is no longer supported. Setting "
                       "IterateEvents=True." << std::endl;
    IterateEvents = true;
  }

  CALL_MDEVENT_FUNCTION(this->binByIterating, m_inWS);

  // Copy the

  // Copy the coordinate system & experiment infos to the output
  IMDEventWorkspace_sptr inEWS =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(m_inWS);
  if (inEWS) {
    outWS->setCoordinateSystem(inEWS->getSpecialCoordinateSystem());
    outWS->copyExperimentInfos(*inEWS);
  }

  outWS->updateSum();
  // Save the output
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));
}

} // namespace Mantid
} // namespace MDEvents
