//--------------------------------
// Includes
//--------------------------------
#include "MantidDataHandling/CreateSampleShape.h"
#include "MantidGeometry/ShapeFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace DataHandling
{
  // Register the algorithm into the AlgorithmFactory
  //  DECLARE_ALGORITHM(CreateSampleShape)
}
}

using namespace Mantid::DataHandling;

// Get a reference to the logger. It is used to print out information, warning and error messages
Mantid::Kernel::Logger& CreateSampleShape::g_log = Mantid::Kernel::Logger::get("CreateSampleShape");



/**
 * Initialize the algorithm
 */
void CreateSampleShape::init()
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("Workspace","",Direction::Input));
  declareProperty("ShapeXML","",new MandatoryValidator<std::string>());
}

/**
 * Execute the algorithm
 */
void CreateSampleShape::exec()
{
  
}

