#include "MantidMDEvents/LoadMDEW.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using Mantid::Geometry::IMDDimensionFactory;
using Mantid::Geometry::IMDDimension_sptr;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadMDEW)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadMDEW::LoadMDEW()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadMDEW::~LoadMDEW()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadMDEW::initDocs()
  {
    this->setWikiSummary("Load a .nxs file into a MDEventWorkspace.");
    this->setOptionalMessage("Load a .nxs file into a MDEventWorkspace.");
    this->setWikiDescription("Load a .nxs file into a MDEventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadMDEW::init()
  {

    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load, as a full or relative path");

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");
  }




  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadMDEW::exec()
  {
    std::string filename = getPropertyValue("Filename");

    // Start loading
    file = new ::NeXus::File(filename);

    file->openGroup("workspace", "NXworkspace");

    int numDimensions = 0;
    file->getAttr("dimensions", numDimensions);
    if (numDimensions <= 0)
      throw std::runtime_error("LoadMDEW:: number of dimensions <= 0.");

    //The type of event
    std::string eventType;
    file->getAttr("event_type", eventType);

    // Done loading that general data
    file->closeGroup();

    // Use the factory to make the workspace of the right type
    IMDEventWorkspace_sptr ws = MDEventFactory::CreateMDEventWorkspace(size_t(numDimensions), eventType);

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->doLoad, ws);

    // Save to output
    setProperty("OutputWorkspace", ws);
  }


  //----------------------------------------------------------------------------------------------
  /** Templated method to do the work
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void LoadMDEW::doLoad(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    file->openGroup("workspace", "NXworkspace");

    // Load each dimension, as their XML representation
    for (size_t d=0; d<nd; d++)
    {
      std::ostringstream mess;
      mess << "dimension" << d;
      std::string dimXML;
      file->getAttr( mess.str(), dimXML );
      // Use the dimension factory to read the XML
      IMDDimensionFactory factory = IMDDimensionFactory::createDimensionFactory(dimXML);
      IMDDimension_sptr dim(factory.create());
      ws->addDimension(dim);
    }

    file->closeGroup();

    file->openGroup("data", "NXdata");

    // Box type (0=None, 1=MDBox, 2=MDGridBox
    std::vector<int> boxType;
    // Recursion depth
    std::vector<int> depth;
    // Start/end indices into the list of events
    std::vector<int64_t> box_event_index;
    // Min/Max extents in each dimension
    std::vector<double> extents;
    // Inverse of the volume of the cell
    std::vector<double> inverseVolume;
    // Box cached signal/error squared
    std::vector<double> box_signal_errorsquared;
    // Start/end children IDs
    std::vector<int> box_children;

    // Read all the data blocks
    file->readData("boxType", boxType);
    file->readData("depth", depth);
    file->readData("inverseVolume", inverseVolume);
    file->readData("extents", extents);
    file->readData("box_children", box_children);
    file->readData("box_signal_errorsquared", box_signal_errorsquared);
    file->readData("box_event_index", box_event_index);

    file->closeGroup();
    file->close();
  }


} // namespace Mantid
} // namespace MDEvents

