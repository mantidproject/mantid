#include "MantidMDEvents/CreateMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateMDEventWorkspace)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateMDEventWorkspace::CreateMDEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateMDEventWorkspace::~CreateMDEventWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateMDEventWorkspace::init()
  {
    setOptionalMessage("Creates an empty MDEventWorkspace with a given number of dimensions.");
    declareProperty(new PropertyWithValue<int>("Dimensions",1,Direction::Input), "Number of dimensions that the workspace will have.");

    std::vector<std::string> propOptions;
    propOptions.push_back("MDEvent");
    declareProperty("EventType", "MDEvent",new ListValidator(propOptions),
      "Which underlying data type will event take (only one option is currently available).");

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateMDEventWorkspace::exec()
  {
    // Get the properties
    std::string eventType = getPropertyValue("EventType");
    int ndims_prop = getProperty("Dimensions");
    if (ndims_prop <= 0)
      throw std::invalid_argument("You must specify a number of dimensions >= 1.");

    size_t ndims = static_cast<size_t>(ndims_prop);

    // Have the factory create it
    IMDEventWorkspace_sptr out = MDEventFactory::CreateMDEventWorkspace(ndims, eventType);

    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(out));
  }



} // namespace Mantid
} // namespace MDEvents

