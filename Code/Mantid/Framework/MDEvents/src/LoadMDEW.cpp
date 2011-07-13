#include "MantidMDEvents/LoadMDEW.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/CPUTimer.h"
#include <vector>

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

    std::vector<size_t> vecDims;
    file->readData("dimensions", vecDims);
    if (vecDims.empty())
      throw std::runtime_error("LoadMDEW:: Error loading number of dimensions.");
    size_t numDims = vecDims[0];
    if (numDims <= 0)
      throw std::runtime_error("LoadMDEW:: number of dimensions <= 0.");

    //The type of event
    std::string eventType;
    file->readData("event_type", eventType);

    // Done loading that general data
    file->closeGroup();

    // Use the factory to make the workspace of the right type
    IMDEventWorkspace_sptr ws = MDEventFactory::CreateMDEventWorkspace(numDims, eventType);

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
    CPUTimer tim;
    bool verbose=true;
    Progress * prog = new Progress(this, 0.0, 1.0, 100);

    prog->report("Opening file.");

    file->openGroup("workspace", "NXworkspace");

    // Load each dimension, as their XML representation
    for (size_t d=0; d<nd; d++)
    {
      std::ostringstream mess;
      mess << "dimension" << d;
      std::string dimXML;
      file->readData(mess.str(), dimXML);
      // Use the dimension factory to read the XML
      IMDDimensionFactory factory = IMDDimensionFactory::createDimensionFactory(dimXML);
      IMDDimension_sptr dim(factory.create());
      ws->addDimension(dim);
    }
    // Load the box controller
    std::string bcXML;
    file->readData("box_controller_xml", bcXML);
    ws->getBoxController()->fromXMLString(bcXML);

    if (verbose) std::cout << tim << " to load the dimensions, etc." << std::endl;

    file->closeGroup();

    file->openGroup("data", "NXdata");

    prog->report("Creating Vectors");

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

    if (verbose) std::cout << tim << " to initialize the box data vectors." << std::endl;
    prog->report("Reading Box Data");

    // Read all the data blocks
    file->readData("boxType", boxType);
    file->readData("depth", depth);
    file->readData("inverseVolume", inverseVolume);
    file->readData("extents", extents);
    file->readData("box_children", box_children);
    file->readData("box_signal_errorsquared", box_signal_errorsquared);
    file->readData("box_event_index", box_event_index);

    if (verbose) std::cout << tim << " to read all the box data vectors." << std::endl;

    size_t numBoxes = boxType.size();
    // Check all vector lengths match
    if (depth.size() != numBoxes) throw std::runtime_error("Incompatible size for data: depth.");
    if (inverseVolume.size() != numBoxes) throw std::runtime_error("Incompatible size for data: inverseVolume.");
    if (boxType.size() != numBoxes) throw std::runtime_error("Incompatible size for data: boxType.");
    if (extents.size() != numBoxes*nd*2) throw std::runtime_error("Incompatible size for data: extents.");
    if (box_children.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_children.");
    if (box_event_index.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_event_index.");
    if (box_signal_errorsquared.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_signal_errorsquared.");

    std::vector<IMDBox<MDE,nd> *> boxes(numBoxes, NULL);
    BoxController_sptr bc = ws->getBoxController();

    prog->setNumSteps(numBoxes);

    // Get ready to read the slabs
    MDE::openNexusData(file);

    size_t totalEvents=0;

    for (size_t i=0; i<numBoxes; i++)
    {
      prog->report();
      size_t box_type = boxType[i];
      if (box_type > 0)
      {
        IMDBox<MDE,nd> * ibox = NULL;

        if (box_type == 1)
        {
          // --- Make a MDBox -----
          MDBox<MDE,nd> * box = new MDBox<MDE,nd>(bc, depth[i]);
          ibox = box;

          // Load the events now
          size_t indexStart = box_event_index[i*2];
          size_t indexEnd = box_event_index[i*2+1];
          if (indexEnd > indexStart)
          {
            //std::cout << "box " << i << " from " << indexStart << " to " << indexEnd << std::endl;
            std::vector<MDE> & events = box->getEvents();
            events.clear();
            MDE::loadVectorFromNexusSlab(events, file, indexStart, indexEnd-indexStart);
            totalEvents += (indexEnd-indexStart);
          }
        }
        else if (box_type == 2)
        {
          // --- Make a MDGridBox -----
          ibox = new MDGridBox<MDE,nd>(bc);
        }
        else
          continue;

        // Force correct ID
        ibox->setId(i);
        // Same box controller as everything else.
        ibox->setBoxController(bc);

        // Extents of the box
        for (size_t d=0; d<nd; d++)
          ibox->setExtents(d, extents[i*2+d*2], extents[i*2+d*2+1]);
        ibox->calcVolume();

        // Save the box at its index in the vector.
        boxes[i] = ibox;
      }
    }

    // Done reading in all the events.
    MDE::closeNexusData(file);

    if (verbose) std::cout << tim << " to create all the boxes and fill them with " << totalEvents << " events." << std::endl;


    totalEvents=0;
    // Go again, giving the children to the parents
    for (size_t i=0; i<numBoxes; i++)
    {
      if (boxType[i] == 2)
      {
        size_t indexStart = box_children[i*2];
        size_t indexEnd = box_children[i*2+1] + 1;
        boxes[i]->setChildren( boxes, indexStart, indexEnd);
      }
      else
        totalEvents += boxes[i]->getNPoints();
    }

    if (verbose) std::cout << tim << " to give all the children to the boxes. They total " << totalEvents << " events." << std::endl;
    std::cout << ws->getNPoints() << " points now." << std::endl;

    // Box of ID 0 is the head box.
    ws->setBox( boxes[0] );
    // Make sure the max ID is ok for later ID generation
    bc->setMaxId(numBoxes);
    // Refresh cache
    ws->refreshCache();
    std::cout << ws->getNPoints() << " points after refresh." << std::endl;

    file->closeGroup();
    file->close();

    if (verbose) std::cout << tim << " to finish up." << std::endl;
    delete prog;
  }


} // namespace Mantid
} // namespace MDEvents

