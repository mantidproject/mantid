//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MoveInstrumentComponent)

/// Sets documentation strings for this algorithm
void MoveInstrumentComponent::initDocs()
{
  this->setWikiSummary(" Moves an instrument component to a new position. ");
  this->setOptionalMessage("Moves an instrument component to a new position.");
}


using namespace Kernel;
using namespace Geometry;
using namespace API;

/// Empty default constructor
MoveInstrumentComponent::MoveInstrumentComponent()
{}

/// Initialisation method.
void MoveInstrumentComponent::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut));
  declareProperty("ComponentName","");
  declareProperty("DetectorID",-1);
  declareProperty("X",0.0);
  declareProperty("Y",0.0);
  declareProperty("Z",0.0);
  declareProperty("RelativePosition",true);
}

/** Executes the algorithm. 
 * 
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void MoveInstrumentComponent::exec()
{
  // Get the workspace
  MatrixWorkspace_sptr WS = getProperty("Workspace");
  const std::string ComponentName = getProperty("ComponentName");
  const int DetID = getProperty("DetectorID");
  const double X = getProperty("X");
  const double Y = getProperty("Y");
  const double Z = getProperty("Z");
  const bool RelativePosition = getProperty("RelativePosition");

  Instrument_const_sptr inst = WS->getInstrument();
  IComponent_const_sptr comp;

  // Find the component to move
  if (DetID != -1)
  {
      comp = inst->getDetector(DetID);
      if (comp == 0)
      {
          std::ostringstream mess;
          mess<<"Detector with ID "<<DetID<<" was not found.";
          g_log.error(mess.str());
          throw std::runtime_error(mess.str());
      }
  }
  else if (!ComponentName.empty())
  {
      comp = inst->getComponentByName(ComponentName);
      if (comp == 0)
      {
          std::ostringstream mess;
          mess<<"Component with name "<<ComponentName<<" was not found.";
          g_log.error(mess.str());
          throw std::runtime_error(mess.str());
      }
  }
  else
  {
      g_log.error("DetectorID or ComponentName must be given.");
      throw std::invalid_argument("DetectorID or ComponentName must be given.");
  }

  V3D Pos;// New relative position
  // First set it to the new absolute position
  if (RelativePosition)
  {
      Pos = comp->getPos() + V3D(X,Y,Z);
  }
  else
  {
      Pos = V3D(X,Y,Z);
  }

  // Then find the corresponding relative position
  boost::shared_ptr<const IComponent> parent = comp->getParent();
  if (parent)
  {
      Pos -= parent->getPos();
      Quat rot = parent->getRelativeRot();
      rot.inverse();
      rot.rotate(Pos);
  }
  boost::shared_ptr<const IComponent>grandparent = parent->getParent();
  if (grandparent)
  {
      Quat rot = grandparent->getRelativeRot();
      rot.inverse();
      rot.rotate(Pos);
  }

  Geometry::ParameterMap& pmap = WS->instrumentParameters();
  // Add a parameter for the new position
  pmap.addV3D(comp.get(), "pos", Pos);

  return;
}

} // namespace DataHandling
} // namespace Mantid
