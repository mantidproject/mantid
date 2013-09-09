/*WIKI*

This algorithm applies a simple linear transformation to a [[MDWorkspace]] or [[MDHistoWorkspace]].
This could be used, for example, to scale the Energy dimension to different units.

Each coordinate is tranformed so that
<math> x'_d = (x_d * s_d) + o_d </math>
where:
* d : index of the dimension, from 0 to the number of dimensions
* s : value of the Scaling parameter
* o : value of the Offset parameter.

You can specify either a single value for Scaling and Offset, in which case the
same m_scaling or m_offset are applied to each dimension; or you can specify a list
with one entry for each dimension.

==== Notes ====

The relationship between the workspace and the original [[MDWorkspace]],
for example when the MDHistoWorkspace is the result of [[BinMD]], is lost.
This means that you cannot re-bin a transformed [[MDHistoWorkspace]].

No units are not modified by this algorithm.

==== Performance Notes ====

* Performing the operation in-place (input=output) is always faster because the first step of the algorithm if NOT in-place is to clone the original workspace.
* For [[MDHistoWorkspace]]s done in-place, TransformMD is very quick (no data is modified, just the coordinates).
* For [[MDWorkspace]]s, every event's coordinates gets modified, so this may take a while for large workspaces.
* For file-backed [[MDWorkspace]]s, you will find much better performance if you perform the change in-place (input=output), because the data gets written out to disk twice otherwise.

*WIKI*/

#include "MantidMDAlgorithms/TransformMD.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using Mantid::MDEvents::MDHistoWorkspace_sptr;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(TransformMD)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  TransformMD::TransformMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TransformMD::~TransformMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string TransformMD::name() const { return "TransformMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int TransformMD::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string TransformMD::category() const { return "MDAlgorithms";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void TransformMD::initDocs()
  {
    this->setWikiSummary("Scale and/or offset the coordinates of a MDWorkspace");
    this->setOptionalMessage("Scale and/or offset the coordinates of a MDWorkspace");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void TransformMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace","",Direction::Input), "Any input MDWorkspace.");

    std::vector<double> defaultScaling(1, 1.0);
    declareProperty(
      new ArrayProperty<double>("Scaling", defaultScaling),
      "Scaling value multiplying each coordinate. Default 1.\nEither a single value or a list for each dimension.");

    std::vector<double> defaultOffset(1, 0.0);
    declareProperty(
      new ArrayProperty<double>("Offset", defaultOffset),
      "Offset value to add to each coordinate. Default 0.\nEither a single value or a list for each dimension.");

    declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output MDWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the transform on a MDEventWorkspace
   *
   * @param ws :: MDEventWorkspace
   */
  template<typename MDE, size_t nd>
  void TransformMD::doTransform(typename Mantid::MDEvents::MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::vector<API::IMDNode * > boxes;
    // Get ALL the boxes, including MDGridBoxes.
    ws->getBox()->getBoxes(boxes, 1000, false);

    // If file backed, sort them first.
    if (ws->isFileBacked())
        API::IMDNode::sortObjByID(boxes);

    PARALLEL_FOR_IF( !ws->isFileBacked() )
    for (int i=0; i<int(boxes.size()); i++)
    {
      PARALLEL_START_INTERUPT_REGION
      MDBoxBase<MDE,nd> * box = dynamic_cast<MDBoxBase<MDE,nd> *>(boxes[i]);
      if (box)
      {
        box->transformDimensions(m_scaling, m_offset);
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void TransformMD::exec()
  {
    Mantid::API::IMDWorkspace_sptr inWS;
    Mantid::API::IMDWorkspace_sptr outWS;

    inWS = getProperty("InputWorkspace");
    outWS = getProperty("OutputWorkspace");

    if (boost::dynamic_pointer_cast<MatrixWorkspace>(inWS))
        throw std::runtime_error("TransformMD can only transform a MDHistoWorkspace or a MDEventWorkspace.");

    if (outWS != inWS)
    {
      // NOT in-place. So first we clone inWS into outWS
      IAlgorithm_sptr clone = this->createChildAlgorithm("CloneMDWorkspace", 0.0, 0.5, true);
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
      throw std::invalid_argument("Scaling argument must be either length 1 or match the number of dimensions.");
    if (m_offset.size() != nd)
      throw std::invalid_argument("Offset argument must be either length 1 or match the number of dimensions.");

    // Transform the dimensions
    outWS->transformDimensions(m_scaling, m_offset);

    MDHistoWorkspace_sptr histo = boost::dynamic_pointer_cast<MDHistoWorkspace>(outWS);
    IMDEventWorkspace_sptr event = boost::dynamic_pointer_cast<IMDEventWorkspace>(outWS);

    if (histo)
    {
      // Recalculate all the values since the dimensions changed.
      histo->cacheValues();
    }
    else if (event)
    {
      // Call the method for this type of MDEventWorkspace.
      CALL_MDEVENT_FUNCTION(this->doTransform, outWS);
    }

    this->setProperty("OutputWorkspace", outWS);
  }



} // namespace Mantid
} // namespace MDAlgorithms
