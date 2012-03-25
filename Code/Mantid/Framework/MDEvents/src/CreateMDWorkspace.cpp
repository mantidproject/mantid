/*WIKI* 

This algorithm creates an empty MDEventWorkspace from scratch. The workspace can have any number of dimensions (up to ~20).
Each dimension must have its name, units, extents specified as comma-spearated string.

The SplitInto parameter determines how splitting of dense boxes will be performed.
For example, if SplitInto=5 and the number of dimensions is 3, then each box will get split into 5x5x5 sub-boxes.

The SplitThreshold parameter determines how many events to keep in a box before splitting it into sub-boxes.
This value can significantly affect performance/memory use!
Too many events per box will mean unnecessary iteration and a slowdown in general.
Too few events per box will waste memory with the overhead of boxes.

You can create a file-backed MDEventWorkspace by specifying the Filename and Memory parameters.

*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/CreateMDWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidKernel/Memory.h"
#include <math.h>
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace MDEvents
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateMDWorkspace)
  
  /// Sets documentation strings for this algorithm
  void CreateMDWorkspace::initDocs()
  {
    this->setWikiSummary("Creates an empty MDEventWorkspace with a given number of dimensions. ");
    this->setOptionalMessage("Creates an empty MDEventWorkspace with a given number of dimensions.");
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateMDWorkspace::CreateMDWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateMDWorkspace::~CreateMDWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateMDWorkspace::init()
  {
    declareProperty(new PropertyWithValue<int>("Dimensions",1,Direction::Input), "Number of dimensions that the workspace will have.");

    std::vector<std::string> propOptions;
    propOptions.push_back("MDEvent");
    propOptions.push_back("MDLeanEvent");
    declareProperty("EventType", "MDLeanEvent",boost::make_shared<StringListValidator>(propOptions),
      "Which underlying data type will event take.");

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

    // Set the box controller properties
    this->initBoxControllerProps("5", 1000, 5);

    declareProperty(
      new PropertyWithValue<int>("MinRecursionDepth", 0),
      "Optional. If specified, then all the boxes will be split to this minimum recursion depth. 0 = no splitting, 1 = one level of splitting, etc.\n"
      "Be careful using this since it can quickly create a huge number of boxes = (SplitInto ^ (MinRercursionDepth * NumDimensions)).");
    setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");

    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::OptionalSave, exts),
        "Optional: to use a file as the back end, give the path to the file to save.");

    declareProperty(new PropertyWithValue<int>("Memory", -1),
        "If Filename is specified to use a file back end:\n"
        "  The amount of memory (in MB) to allocate to the in-memory cache.\n"
        "  If not specified, a default of 40% of free physical memory is used.");
    setPropertySettings("Memory", new EnabledWhenProperty(this, "Filename", IS_NOT_DEFAULT));

  }


  /** Finish initialisation
   *
   * @param ws :: MDEventWorkspace to finish
   */
  template<typename MDE, size_t nd>
  void CreateMDWorkspace::finish(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // ------------ Set up the box controller ----------------------------------
    BoxController_sptr bc = ws->getBoxController();
    this->setBoxController(bc);

    // Split to level 1
    ws->splitBox();

    // Do we split more due to MinRecursionDepth?
    int minDepth = this->getProperty("MinRecursionDepth");
    if (minDepth<0) throw std::invalid_argument("MinRecursionDepth must be >= 0.");
    ws->setMinRecursionDepth(size_t(minDepth));
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateMDWorkspace::exec()
  {
    // Get the properties and validate them
    std::string eventType = getPropertyValue("EventType");
    int ndims_prop = getProperty("Dimensions");
    if (ndims_prop <= 0)
      throw std::invalid_argument("You must specify a number of dimensions >= 1.");
    int mind = this->getProperty("MinRecursionDepth");
    int maxd = this->getProperty("MaxRecursionDepth");
    if (mind > maxd)
      throw std::invalid_argument("MinRecursionDepth must be <= MaxRecursionDepth.");
    if (mind < 0 || maxd < 0)
      throw std::invalid_argument("MinRecursionDepth and MaxRecursionDepth must be positive.");

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
    IMDEventWorkspace_sptr out = MDEventFactory::CreateMDWorkspace(ndims, eventType);

    // Give all the dimensions
    for (size_t d=0; d<ndims; d++)
    {
      MDHistoDimension * dim = new MDHistoDimension(names[d], names[d], units[d], static_cast<coord_t>(extents[d*2]), static_cast<coord_t>(extents[d*2+1]), 1);
      out->addDimension(MDHistoDimension_sptr(dim));
    }

    // Initialize it using the dimension
    out->initialize();

    // Call the templated function to finish ints
    CALL_MDEVENT_FUNCTION(this->finish, out);

    // --- File back end ? ----------------
    std::string filename = getProperty("Filename");
    if (!filename.empty())
    {
      // First save to the NXS file
      g_log.notice() << "Running SaveMD" << std::endl;
      IAlgorithm_sptr alg = createSubAlgorithm("SaveMD");
      alg->setPropertyValue("Filename", filename);
      alg->setProperty("InputWorkspace", out);
      alg->executeAsSubAlg();
      // And now re-load it with this file as the backing.
      g_log.notice() << "Running LoadMD" << std::endl;
      alg = createSubAlgorithm("LoadMD");
      alg->setPropertyValue("Filename", filename);
      alg->setProperty("FileBackEnd", true);
      alg->setPropertyValue("Memory", getPropertyValue("Memory"));
      alg->executeAsSubAlg();
      // Replace the workspace with the loaded, file-backed one
      out = alg->getProperty("OutputWorkspace");
    }

    // Save it on the output.
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(out));
  }



} // namespace Mantid
} // namespace MDEvents

