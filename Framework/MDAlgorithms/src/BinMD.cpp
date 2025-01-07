// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/BinMD.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidDataObjects/CoordTransformAffineParser.h"
#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Utils.h"
#include <boost/algorithm/string.hpp>

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(BinMD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
BinMD::BinMD()
    : SlicingAlgorithm(), outWS(), implicitFunction(), indexMultiplier(), signals(nullptr), errors(nullptr),
      numEvents(nullptr) {}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void BinMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDWorkspace.");

  // Properties for specifying the slice to perform.
  this->initSlicingProps();

  // --------------- Processing methods and options
  // ---------------------------------------
  std::string grp = "Methods";
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("ImplicitFunctionXML", "", Direction::Input),
                  "XML string describing the implicit function determining "
                  "which bins to use.");
  setPropertyGroup("ImplicitFunctionXML", grp);

  declareProperty(std::make_unique<PropertyWithValue<bool>>("IterateEvents", true, Direction::Input),
                  "Alternative binning method where you iterate through every event, "
                  "placing them in the proper bin.\n"
                  "This may be faster for workspaces with few events and lots of output "
                  "bins.");
  setPropertyGroup("IterateEvents", grp);

  declareProperty(std::make_unique<PropertyWithValue<bool>>("Parallel", false, Direction::Input),
                  "Temporary parameter: true to run in parallel. This is ignored for "
                  "file-backed workspaces, where running in parallel makes things slower "
                  "due to disk thrashing.");
  setPropertyGroup("Parallel", grp);

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryDataWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "An input MDHistoWorkspace used to accumulate results from "
                  "multiple MDEventWorkspaces. If unspecified a blank "
                  "MDHistoWorkspace will be created.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
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
inline void BinMD::binMDBox(MDBox<MDE, nd> *box, const size_t *const chunkMin, const size_t *const chunkMax) {
  // An array to hold the rotated/transformed coordinates
  auto outCenter = std::vector<coord_t>(m_outD);

  // Evaluate whether the entire box is in the same bin
  if (box->getNPoints() > (1 << nd) * 2) {
    // There is a check that the number of events is enough for it to make sense
    // to do all this processing.
    size_t numVertexes = 0;
    auto vertexes = box->getVertexesArray(numVertexes);

    // All vertexes have to be within THE SAME BIN = have the same linear index.
    size_t lastLinearIndex = 0;
    bool badOne = false;

    for (size_t i = 0; i < numVertexes; i++) {
      // Cache the center of the event (again for speed)
      const coord_t *inCenter = vertexes.get() + i * nd;

      // Now transform to the output dimensions
      m_transform->apply(inCenter, outCenter.data());

      // To build up the linear index
      size_t linearIndex = 0;
      // To mark VERTEXES outside range
      badOne = false;

      /// Loop through the dimensions on which we bin
      for (size_t bd = 0; bd < m_outD; bd++) {
        // What is the bin index in that dimension
        coord_t x = outCenter[bd];
        auto ix = size_t(x);
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

    if (!badOne) {
      // Yes, the entire box is within a single bin
      //        std::cout << "Box at " << box->getExtentsStr() << " is within a
      //        single bin.\n";
      // Add the CACHED signal from the entire box
      signals[lastLinearIndex] += box->getSignal();
      errors[lastLinearIndex] += box->getErrorSquared();
      // TODO: If DataObjects get a weight, this would need to get the summed
      // weight.
      numEvents[lastLinearIndex] += static_cast<signal_t>(box->getNPoints());

      // And don't bother looking at each event. This may save lots of time
      // loading from disk.
      return;
    }
  }

  // If you get here, you could not determine that the entire box was in the
  // same bin.
  // So you need to iterate through events.
  const std::vector<MDE> &events = box->getConstEvents();
  for (auto it = events.begin(); it != events.end(); ++it) {
    // Cache the center of the event (again for speed)
    const coord_t *inCenter = it->getCenter();

    // Now transform to the output dimensions
    m_transform->apply(inCenter, outCenter.data());

    // To build up the linear index
    size_t linearIndex = 0;
    // To mark events outside range
    bool badOne = false;

    /// Loop through the dimensions on which we bin
    for (size_t bd = 0; bd < m_outD; bd++) {
      // What is the bin index in that dimension
      coord_t x = outCenter[bd];
      auto ix = size_t(x);
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
      // TODO: If DataObjects get a weight, this would need to get the summed
      // weight.
      numEvents[linearIndex] += 1.0;
    }
  }
  // Done with the events list
  box->releaseEvents();
}

//----------------------------------------------------------------------------------------------
/** Perform binning by iterating through every event and placing them in the
 *output workspace
 *
 * @param ws :: MDEventWorkspace of the given type.
 */
template <typename MDE, size_t nd> void BinMD::binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  BoxController_sptr bc = ws->getBoxController();
  // store exisiting write buffer size for the future
  // uint64_t writeBufSize =bc->getDiskBuffer().getWriteBufferSize();
  // and disable write buffer (if any) for input MD Events for this algorithm
  // purposes;
  // bc->setCacheParameters(1,0);

  // Cache some data to speed up accessing them a bit
  indexMultiplier.resize(m_outD);
  for (size_t d = 0; d < m_outD; d++) {
    if (d > 0)
      indexMultiplier[d] = outWS->getIndexMultiplier()[d - 1];
    else
      indexMultiplier[d] = 1;
  }
  signals = outWS->mutableSignalArray();
  errors = outWS->mutableErrorSquaredArray();
  numEvents = outWS->mutableNumEventsArray();

  if (!m_accumulate) {
    // Start with signal/error/numEvents at 0.0
    outWS->setTo(0.0, 0.0, 0.0);
  }

  // The dimension (in the output workspace) along which we chunk for parallel
  // processing
  // TODO: Find the smartest dimension to chunk against
  size_t chunkDimension = 0;

  // How many bins (in that dimension) per chunk.
  // Try to split it so each core will get 2 tasks:
  auto chunkNumBins = int(m_binDimensions[chunkDimension]->getNBins() / (PARALLEL_GET_MAX_THREADS * 2));
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
  if (prog) {
    prog->setNotifyStep(0.1);
    prog->resetNumSteps(100, 0.00, 1.0);
  }

  // Run the chunks in parallel. There is no overlap in the output workspace so
  // it is thread safe to write to it..

    PRAGMA_OMP( parallel for schedule(dynamic,1) if (doParallel) )
    for (int chunk = 0; chunk < int(m_binDimensions[chunkDimension]->getNBins()); chunk += chunkNumBins) {
      PARALLEL_START_INTERRUPT_REGION
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
      if (size_t(chunk + chunkNumBins) > m_binDimensions[chunkDimension]->getNBins())
        chunkMax[chunkDimension] = m_binDimensions[chunkDimension]->getNBins();
      else
        chunkMax[chunkDimension] = size_t(chunk + chunkNumBins);

      // Build an implicit function (it needs to be in the space of the
      // MDEventWorkspace)
      auto function = this->getImplicitFunctionForChunk(chunkMin.data(), chunkMax.data());

      // Use getBoxes() to get an array with a pointer to each box
      std::vector<API::IMDNode *> boxes;
      // Leaf-only; no depth limit; with the implicit function passed to it.
      ws->getBox()->getBoxes(boxes, 1000, true, function.get());

      // Sort boxes by file position IF file backed. This reduces seeking time,
      // hopefully.
      if (bc->isFileBacked())
        API::IMDNode::sortObjByID(boxes);

      // For progress reporting, the # of boxes
      if (prog) {
        PARALLEL_CRITICAL(BinMD_progress) {
          g_log.debug() << "Chunk " << chunk << ": found " << boxes.size() << " boxes within the implicit function.\n";
          progNumSteps += boxes.size();
          prog->setNumSteps(progNumSteps);
        }
      }

      // Go through every box for this chunk.
      for (auto &boxe : boxes) {
        auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxe);
        // Perform the binning in this separate method.
        if (box && !box->getIsMasked())
          this->binMDBox(box, chunkMin.data(), chunkMax.data());

        // Progress reporting
        if (prog)
          prog->report();
        // For early cancelling of the loop
        if (this->m_cancel)
          break;
      } // for each box in the vector
      PARALLEL_END_INTERRUPT_REGION
    } // for each chunk in parallel
    PARALLEL_CHECK_INTERRUPT_REGION

    // Now the implicit function
    if (implicitFunction) {
      if (prog)
        prog->report("Applying implicit function.");
      signal_t nan = std::numeric_limits<signal_t>::quiet_NaN();
      outWS->applyImplicitFunction(implicitFunction.get(), nan, nan);
    }
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
  implicitFunction = nullptr;
  if (!ImplicitFunctionXML.empty())
    implicitFunction = std::unique_ptr<MDImplicitFunction>(
        Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(ImplicitFunctionXML));

  // This gets deleted by the thread pool; don't delete it in here.
  prog = std::make_unique<Progress>(this, 0.0, 1.0, 1);

  // Create the dense histogram. This allocates the memory
  std::shared_ptr<IMDHistoWorkspace> tmp = this->getProperty("TemporaryDataWorkspace");
  outWS = std::dynamic_pointer_cast<MDHistoWorkspace>(tmp);
  if (!outWS) {
    outWS = std::make_shared<MDHistoWorkspace>(m_binDimensions);
  } else {
    m_accumulate = true;
  }

  // Saves the geometry transformation from original to binned in the workspace
  outWS->setTransformFromOriginal(this->m_transformFromOriginal.release(), 0);
  outWS->setTransformToOriginal(this->m_transformToOriginal.release(), 0);
  for (size_t i = 0; i < m_bases.size(); i++)
    outWS->setBasisVector(i, m_bases[i]);
  outWS->setOrigin(this->m_translation);
  outWS->setOriginalWorkspace(m_inWS, 0);

  // And the intermediate WS one too, if any
  if (m_intermediateWS) {
    outWS->setOriginalWorkspace(m_intermediateWS, 1);
    outWS->setTransformFromOriginal(m_transformFromIntermediate.release(), 1);
    outWS->setTransformToOriginal(m_transformToIntermediate.release(), 1);
  }

  // Wrapper to cast to MDEventWorkspace then call the function
  bool IterateEvents = getProperty("IterateEvents");
  if (!IterateEvents) {
    g_log.warning() << "IterateEvents=False is no longer supported. Setting "
                       "IterateEvents=True.\n";
    IterateEvents = true;
  }

  /*
  We should fail noisily here. CALL_MDEVENT_FUNCTION will silently allow
  IMDHistoWorkspaces to cascade through to the end
  and result in an empty output. The only way we allow InputWorkspaces to be
  IMDHistoWorkspaces is if they also happen to contain original workspaces
  that are MDEventWorkspaces.
  */
  if (m_inWS->isMDHistoWorkspace()) {
    throw std::runtime_error("Cannot rebin a workspace that is histogrammed and has no original "
                             "workspace that is an MDEventWorkspace. "
                             "Reprocess the input so that it contains full MDEvents.");
  }

  CALL_MDEVENT_FUNCTION(this->binByIterating, m_inWS);

  // Copy the coordinate system & experiment infos to the output
  IMDEventWorkspace_sptr inEWS = std::dynamic_pointer_cast<IMDEventWorkspace>(m_inWS);
  if (inEWS) {
    outWS->setCoordinateSystem(inEWS->getSpecialCoordinateSystem());
    try {
      outWS->copyExperimentInfos(*inEWS);
    } catch (std::runtime_error &) {
      g_log.warning() << this->name() << " was not able to copy experiment info to output workspace "
                      << outWS->getName() << '\n';
    }
  }

  // Pass on the display normalization from the input workspace
  outWS->setDisplayNormalization(m_inWS->displayNormalizationHisto());

  outWS->updateSum();
  // Save the output
  setProperty("OutputWorkspace", std::dynamic_pointer_cast<Workspace>(outWS));
}

} // namespace Mantid::MDAlgorithms
