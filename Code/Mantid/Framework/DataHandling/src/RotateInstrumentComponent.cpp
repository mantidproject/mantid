/*WIKI* 


RotateInstrumentComponent rotates a component around an axis of rotation by an angle given in degrees. Rotation by 0 degrees does not change the component's orientation. The rotation axis (X,Y,Z) must be given in the co-ordinate system attached to the component and rotates with it.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/RotateInstrumentComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(RotateInstrumentComponent)

/// Sets documentation strings for this algorithm
void RotateInstrumentComponent::initDocs()
{
  this->setWikiSummary(" Rotates an instrument component. ");
  this->setOptionalMessage("Rotates an instrument component.");
}


using namespace Kernel;
using namespace Geometry;
using namespace API;

/// Empty default constructor
RotateInstrumentComponent::RotateInstrumentComponent()
{}

/// Initialisation method.
void RotateInstrumentComponent::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut));
  declareProperty("ComponentName","");
  declareProperty("DetectorID",-1);
  declareProperty("X",0.0);
  declareProperty("Y",0.0);
  declareProperty("Z",0.0);
  declareProperty("Angle",0.0);
  declareProperty("RelativeRotation",true);
}

/** Executes the algorithm. 
 * 
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void RotateInstrumentComponent::exec()
{
  // Get the workspace
  MatrixWorkspace_sptr WS = getProperty("Workspace");
  const std::string ComponentName = getProperty("ComponentName");
  const int DetID = getProperty("DetectorID");
  const double X = getProperty("X");
  const double Y = getProperty("Y");
  const double Z = getProperty("Z");
  const double angle = getProperty("Angle");
  const bool RelativeRotation = getProperty("RelativeRotation");

  if (X + Y + Z == 0.0) throw std::invalid_argument("The rotation axis must not be a zero vector");

  // Get the ParameterMap reference before the instrument so that
  // we avoid a copy
  Geometry::ParameterMap& pmap = WS->instrumentParameters();
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

  // First set new relative or absolute rotation
  Quat Rot;
  if (RelativeRotation)
  {
      Quat Rot0 = comp->getRelativeRot();
      Rot = Rot0 * Quat(angle,V3D(X,Y,Z));
  }
  else
  {
      Rot = Quat(angle,V3D(X,Y,Z));
      // Then find the corresponding relative position
      boost::shared_ptr<const IComponent> parent = comp->getParent();
      if (parent)
      {
          Quat rot0 = parent->getRelativeRot();
          rot0.inverse();
          Rot = Rot * rot0;
      }
  }

  // Add a parameter for the new rotation
  pmap.addQuat(comp.get(), "rot", Rot);

  return;
}

} // namespace DataHandling
} // namespace Mantid
