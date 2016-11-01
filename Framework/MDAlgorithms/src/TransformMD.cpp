#include "MantidMDAlgorithms/TransformMD.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(TransformMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string TransformMD::name() const { return "TransformMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int TransformMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string TransformMD::category() const {
  return "MDAlgorithms\\Transforms";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void TransformMD::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Any input MDWorkspace.");

  std::vector<double> defaultScaling(1, 1.0);
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("Scaling", defaultScaling),
      "Scaling value multiplying each coordinate. Default "
      "1.\nEither a single value or a list for each dimension.");

  std::vector<double> defaultOffset(1, 0.0);
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("Offset", defaultOffset),
      "Offset value to add to each coordinate. Default 0.\nEither "
      "a single value or a list for each dimension.");

  declareProperty(make_unique<WorkspaceProperty<IMDWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output MDWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Perform the transform on a MDEventWorkspace
 *
 * @param ws :: MDEventWorkspace
 */
template <typename MDE, size_t nd>
void TransformMD::doTransform(
    typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {
  std::vector<API::IMDNode *> boxes;
  // Get ALL the boxes, including MDGridBoxes.
  ws->getBox()->getBoxes(boxes, 1000, false);

  // If file backed, sort them first.
  if (ws->isFileBacked())
    API::IMDNode::sortObjByID(boxes);

  PARALLEL_FOR_IF(!ws->isFileBacked())
  for (int i = 0; i < static_cast<int>(boxes.size()); i++) { // NOLINT
    PARALLEL_START_INTERUPT_REGION
    MDBoxBase<MDE, nd> *box = dynamic_cast<MDBoxBase<MDE, nd> *>(boxes[i]);
    if (box) {
      box->transformDimensions(m_scaling, m_offset);
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

//----------------------------------------------------------------------------------------------
/** Swap the array elements
*
* @param array :: signal array
* @param arrayLength :: length of signal array
*/
void TransformMD::reverse(signal_t *array, size_t arrayLength) {
  for (size_t i = 0; i < (arrayLength / 2); i++) {
    signal_t temp = array[i];
    array[i] = array[(arrayLength - 1) - i];
    array[(arrayLength - 1) - i] = temp;
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void TransformMD::exec() {
  Mantid::API::IMDWorkspace_sptr inWS;
  Mantid::API::IMDWorkspace_sptr outWS;

  inWS = getProperty("InputWorkspace");
  outWS = getProperty("OutputWorkspace");
  std::string outName = getPropertyValue("OutputWorkspace");

  if (boost::dynamic_pointer_cast<MatrixWorkspace>(inWS))
    throw std::runtime_error("TransformMD can only transform a "
                             "MDHistoWorkspace or a MDEventWorkspace.");

  if (outWS != inWS) {
    // NOT in-place. So first we clone inWS into outWS
    IAlgorithm_sptr clone =
        this->createChildAlgorithm("CloneMDWorkspace", 0.0, 0.5, true);
    clone->setProperty("InputWorkspace", inWS);
    clone->executeAsChildAlg();
    outWS = clone->getProperty("OutputWorkspace");
  }

  if (!outWS)
    throw std::runtime_error("Invalid output workspace.");

  size_t nd = outWS->getNumDims();
  m_scaling = getProperty("Scaling");
  m_offset = getProperty("Offset");

  // Replicate single values
  if (m_scaling.size() == 1)
    m_scaling = std::vector<double>(nd, m_scaling[0]);
  if (m_offset.size() == 1)
    m_offset = std::vector<double>(nd, m_offset[0]);

  // Check the size
  if (m_scaling.size() != nd)
    throw std::invalid_argument("Scaling argument must be either length 1 or "
                                "match the number of dimensions.");
  if (m_offset.size() != nd)
    throw std::invalid_argument("Offset argument must be either length 1 or "
                                "match the number of dimensions.");

  // Transform the dimensions
  outWS->transformDimensions(m_scaling, m_offset);

  MDHistoWorkspace_sptr histo =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(outWS);
  IMDEventWorkspace_sptr event =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(outWS);

  if (histo) {
    // Recalculate all the values since the dimensions changed.
    histo->cacheValues();
    if (m_scaling[0] < 0.0) {
      signal_t *signals = histo->getSignalArray();
      signal_t *errorsSq = histo->getErrorSquaredArray();

      this->reverse(signals, histo->getNPoints());
      this->reverse(errorsSq, histo->getNPoints());
    }

    this->setProperty("OutputWorkspace", histo);
  } else if (event) {
    // Call the method for this type of MDEventWorkspace.
    CALL_MDEVENT_FUNCTION(this->doTransform, outWS);
    Progress *prog2 = nullptr;
    ThreadScheduler *ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts, 0, prog2);
    event->splitAllIfNeeded(ts);
    // prog2->resetNumSteps( ts->size(), 0.4, 0.6);
    tp.joinAll();
    event->refreshCache();
    // Set the special coordinate system.
    IMDEventWorkspace_sptr inEvent =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(inWS);
    event->setCoordinateSystem(inEvent->getSpecialCoordinateSystem());

    if (m_scaling[0] < 0) {
      // Only need these 2 algorithms for transforming with negative number
      std::vector<double> extents;
      std::vector<std::string> names, units;
      for (size_t d = 0; d < nd; d++) {
        Geometry::IMDDimension_const_sptr dim = event->getDimension(d);
        // Find the extents
        extents.push_back(dim->getMinimum());
        extents.push_back(dim->getMaximum());
        names.push_back(std::string(dim->getName()));
        units.push_back(dim->getUnits());
      }
      Algorithm_sptr create_alg = createChildAlgorithm("CreateMDWorkspace");
      create_alg->setProperty("Dimensions", static_cast<int>(nd));
      create_alg->setProperty("EventType", event->getEventTypeName());
      create_alg->setProperty("Extents", extents);
      create_alg->setProperty("Names", names);
      create_alg->setProperty("Units", units);
      create_alg->setPropertyValue("OutputWorkspace", "__none");
      create_alg->executeAsChildAlg();
      Workspace_sptr none = create_alg->getProperty("OutputWorkspace");

      AnalysisDataService::Instance().addOrReplace(outName, event);
      AnalysisDataService::Instance().addOrReplace("__none", none);
      Mantid::API::BoxController_sptr boxController = event->getBoxController();
      std::vector<int> splits;
      for (size_t d = 0; d < nd; d++) {
        splits.push_back(static_cast<int>(boxController->getSplitInto(d)));
      }
      Algorithm_sptr merge_alg = createChildAlgorithm("MergeMD");
      merge_alg->setPropertyValue("InputWorkspaces", outName + ",__none");
      merge_alg->setProperty("SplitInto", splits);
      merge_alg->setProperty(
          "SplitThreshold",
          static_cast<int>(boxController->getSplitThreshold()));
      merge_alg->setProperty("MaxRecursionDepth", 13);
      merge_alg->executeAsChildAlg();
      event = merge_alg->getProperty("OutputWorkspace");
      AnalysisDataService::Instance().remove("__none");
    }
    this->setProperty("OutputWorkspace", event);
  }
}

} // namespace Mantid
} // namespace MDAlgorithms
