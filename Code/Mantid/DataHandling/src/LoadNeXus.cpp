// LoadNeXus
// @author Freddie Akeroyd, STFC ISIS Faility
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadNeXus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "LoadNeXus/NeXusUtils.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(LoadNeXus)

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  Logger& LoadNeXus::g_log = Logger::get("LoadNeXus");

  /// Empty default constructor
  LoadNeXus::LoadNeXus()
  {
  }

  /** Initialisation method.
   * 
   */
  void LoadNeXus::init()
  {
    declareProperty("Filename","",new MandatoryValidator);
    declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output));
  }
  
  /** Executes the algorithm. Reading in the file and creating and populating
   *  the output workspace
   * 
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void LoadNeXus::exec()
  {
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("Filename");
    
    // Get a pointer to the workspace factory (later will be shared)
    API::WorkspaceFactory *factory = API::WorkspaceFactory::Instance();
    m_localWorkspace = boost::dynamic_pointer_cast<Workspace2D>(factory->create("Workspace2D"));

    // Assign the result to the output workspace property
    setProperty("OutputWorkspace",m_localWorkspace);

    testNX();
    
    return;
  }

} // namespace DataHandling
} // namespace Mantid
