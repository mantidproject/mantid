/*WIKI*

This algorithm will resize a [[RectangularDetector]] by applying X and Y scaling factors.
Each pixel's position will be modifed relative to the 0,0 point of the detector by these factors.
Typically, a RectangularDetector is constructed around its center, so this would scale the detector around its center.

This only works on [[RectangularDetector]]s. Banks formed by e.g. tubes cannot be scaled in this way.

Internally, this sets the "scalex" and "scaley" parameters on the [[RectangularDetector]].
Note that the scaling is relative to the original size, and is not cumulative: that is,
if you Resize * 2 and again * 3, your final detector is 3 times larger than the original, not 6 times.

Note: As of this writing, the algorithm does NOT modify the shape of individual pixels. This means
that algorithms based on solid angle calculations might be off.
Ray-tracing (e.g. peak finding) are unaffected.

See also [[MoveInstrumentComponent]] and [[RotateInstrumentComponent]] for other ways to move components.

*WIKI*/

#include "MantidAlgorithms/ResizeRectangularDetector.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ResizeRectangularDetector)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ResizeRectangularDetector::ResizeRectangularDetector()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ResizeRectangularDetector::~ResizeRectangularDetector()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ResizeRectangularDetector::name() const { return "ResizeRectangularDetector";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ResizeRectangularDetector::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ResizeRectangularDetector::category() const { return "DataHandling\\Instrument";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ResizeRectangularDetector::initDocs()
  {
    this->setWikiSummary("Resize a RectangularDetector in X and/or Y.");
    this->setOptionalMessage("Resize a RectangularDetector in X and/or Y.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ResizeRectangularDetector::init()
  {
    // When used as a Child Algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut));
    declareProperty("ComponentName","",
        "The name of the RectangularDetector to resize.");
    declareProperty("ScaleX", 1.0,
        "The scaling factor in the X direction. Default 1.0");
    declareProperty("ScaleY", 1.0,
        "The scaling factor in the Y direction. Default 1.0");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ResizeRectangularDetector::exec()
  {
    MatrixWorkspace_sptr WS = getProperty("Workspace");
    std::string ComponentName = getPropertyValue("ComponentName");
    double ScaleX = getProperty("ScaleX");
    double ScaleY = getProperty("ScaleY");

    if (ComponentName.empty())
      throw std::runtime_error("You must specify a ComponentName.");

    Instrument_const_sptr inst = WS->getInstrument();
    IComponent_const_sptr comp;

    comp = inst->getComponentByName(ComponentName);
    if (!comp)
      throw std::runtime_error("Component with name " +  ComponentName + " was not found.");

    RectangularDetector_const_sptr det = boost::dynamic_pointer_cast<const RectangularDetector>(comp);
    if (!det)
      throw std::runtime_error("Component with name " +  ComponentName + " is not a RectangularDetector.");

    Geometry::ParameterMap& pmap = WS->instrumentParameters();
    // Add a parameter for the new scale factors
    pmap.addDouble(det->getComponentID(), "scalex", ScaleX);
    pmap.addDouble(det->getComponentID(), "scaley", ScaleY);

    pmap.clearCache();
  }



} // namespace Mantid
} // namespace Algorithms
