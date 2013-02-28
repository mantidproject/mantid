/*WIKI* 


Intended for use on data from engineering beamlines, this algorithm creates a shape object for use as the 'gauge volume' (i.e. the portion of the sample that is visible
to the detectors in a given run) of a larger sample in the
[[AbsorptionCorrection]] algorithm. The sample shape will also need to be defined using, e.g., the [[CreateSampleShape]] algorithm. Shapes are defined using XML descriptions that can be found [[HowToDefineGeometricShape|here]]. 

Internally, this works by attaching the XML string (after validating it) to a property called "GaugeVolume" on the workspace's [[Run]] object.


*WIKI*/
//--------------------------------
// Includes
//--------------------------------
#include "MantidDataHandling/DefineGaugeVolume.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid
{
namespace DataHandling
{

using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DefineGaugeVolume)

/// Sets documentation strings for this algorithm
void DefineGaugeVolume::initDocs()
{
  this->setWikiSummary(" Defines a geometrical shape object to be used as the gauge volume in the [[AbsorptionCorrection]] algorithm. ");
  this->setOptionalMessage("Defines a geometrical shape object to be used as the gauge volume in the AbsorptionCorrection algorithm.");
}


/**
 * Initialize the algorithm
 */
void DefineGaugeVolume::init()
{
  declareProperty(new WorkspaceProperty<>("Workspace","",Kernel::Direction::InOut),
    "The workspace with which to associate the defined gauge volume");
  declareProperty("ShapeXML","",boost::make_shared<Kernel::MandatoryValidator<std::string> >(),
    "The XML that describes the shape of the gauge volume" );
}

/**
 * Execute the algorithm
 */
void DefineGaugeVolume::exec()
{  
  // Get the XML definition
  const std::string shapeXML = getProperty("ShapeXML");
  // Try creating the shape to make sure the input's valid
  boost::shared_ptr<Geometry::Object> shape_sptr = Geometry::ShapeFactory().createShape(shapeXML);
  if ( !shape_sptr->hasValidShape() )
  {
    g_log.error("Invalid shape definition provided. Gauge Volume NOT added to workspace.");
    throw std::invalid_argument("Invalid shape definition provided.");
  }

  // Should we check that the volume defined is within the sample? Is this necessary?
  // Do we even have a way to do this?

  progress(0.5);

  // Add as an entry in the workspace's Run object, just as text. Overwrite if already present.
  const MatrixWorkspace_sptr workspace = getProperty("Workspace");
  workspace->mutableRun().addProperty("GaugeVolume",shapeXML,true);

  progress(1);
}

} // namespace DataHandling
} // namespace Mantid
