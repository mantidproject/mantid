//--------------------------------
// Includes
//--------------------------------
#include "MantidDataHandling/CreateSampleShape.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"

namespace Mantid
{
namespace DataHandling
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateSampleShape)
}
}

using namespace Mantid::DataHandling;
using namespace Mantid::API;

/**
 * Initialize the algorithm
 */
void CreateSampleShape::init()
{
  this->setWikiSummary("Create a shape object to model the sample.");
  this->setOptionalMessage("Create a shape object to model the sample.");

  using namespace Mantid::Kernel;
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "The workspace with which to associate the sample ");
  declareProperty("ShapeXML","",new MandatoryValidator<std::string>(),
    "The XML that describes the shape" );
}

/**
 * Execute the algorithm
 */
void CreateSampleShape::exec()
{  
  // Get the input workspace
  MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  // Get the XML definition
  std::string shapeXML = getProperty("ShapeXML");
  Geometry::ShapeFactory sFactory;
  // Create the object
  boost::shared_ptr<Geometry::Object> shape_sptr = sFactory.createShape(shapeXML);
  // Check it's valid and attach it to the workspace sample
  if( shape_sptr->hasValidShape() )
  {
    workspace->mutableSample().setShape(*shape_sptr);
  }
  else
  {
    g_log.warning() << "Object has invalid shape. TopRule = " << shape_sptr->topRule() 
		    << ", number of surfaces = " << shape_sptr->getSurfacePtr().size() << "\n";
    throw std::runtime_error("Shape object is invalid, cannot attach it to workspace."); 
  }
  // Done!
  progress(1);
}

