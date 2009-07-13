//--------------------------------
// Includes
//--------------------------------
#include "MantidDataHandling/CreateSampleShape.h"
#include "MantidGeometry/ShapeFactory.h"
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
  const MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  // Get the XML definition
  std::string shapeXML = getProperty("ShapeXML");
  Geometry::ShapeFactory sFactory;
  boost::shared_ptr<Geometry::Object> shape_sptr = sFactory.createShape(shapeXML);

  workspace->getSample()->setGeometry(shape_sptr);
  progress(1);
}

