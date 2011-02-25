#include "MantidMDEvents/CreateMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
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

    declareProperty(
      new ArrayProperty<double>("Extents"),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension.");

    declareProperty(
      new ArrayProperty<std::string>("Names"),
      "A comma separated list of the name of each dimension.");

    declareProperty(
      new ArrayProperty<std::string>("Units"),
      "A comma separated list of the units of each dimension.");

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

    std::vector<double> extents = getProperty("Extents");
    std::vector<std::string> names = getProperty("Names");
    std::vector<std::string> units = getProperty("Units");

    if (extents.size() != ndims*2)
      throw std::invalid_argument("You must specify twice as many extents (min,max) as there are dimensions.");
    if (names.size() != ndims)
      throw std::invalid_argument("You must specify as many names as there are dimensions.");
    if (units.size() != ndims)
      throw std::invalid_argument("You must specify as many units as there are dimensions.");

    // Have the factory create it
    IMDEventWorkspace_sptr out = MDEventFactory::CreateMDEventWorkspace(ndims, eventType);

    // Give all the dimensions
    for (size_t d=0; d<ndims; d++)
    {
      Dimension dim(extents[d*2], extents[d*2+1], names[d], units[d]);
      out->addDimension(dim);
    }

    // Initialize it using the dimension


    // Save it on the output.
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(out));
  }



} // namespace Mantid
} // namespace MDEvents

