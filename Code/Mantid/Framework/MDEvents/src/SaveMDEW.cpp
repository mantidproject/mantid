#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/SaveMDEW.h"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidMDEvents/MDBox.h"
#include "MantidAPI/Progress.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveMDEW)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveMDEW::SaveMDEW()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveMDEW::~SaveMDEW()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveMDEW::initDocs()
  {
    this->setWikiSummary("Save a MDEventWorkspace to a .nxs file.");
    this->setOptionalMessage("Save a MDEventWorkspace to a .nxs file.");
    this->setWikiDescription("Save a MDEventWorkspace to a .nxs file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveMDEW::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");

    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The name of the Nexus file to write, as a full or relative path");
  }

  //----------------------------------------------------------------------------------------------
  /** Templated method to do the work
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void SaveMDEW::doSave(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::string filename = getPropertyValue("Filename");

    // Open/create the file
    ::NeXus::File * file;
    file = new ::NeXus::File(filename, NXACC_CREATE5);

    // Write out some general information like # of dimensions
    file->makeGroup("workspace", "NXcollection", 1);
    file->writeData("dimensions", int32_t(nd));
    file->writeData("event_type", MDE::getTypeName());

    // Save each dimension, as their XML representation
    for (size_t d=0; d<nd; d++)
    {
      std::ostringstream mess;
      mess << "dimension" << d;
      file->writeData( mess.str(), ws->getDimension(d)->toXMLString() );
    }

    // Add box controller info.
    file->writeData("box_controller_xml", ws->getBoxController()->toXMLString());

    file->closeGroup();

    // Start the main data group
    file->makeGroup("data", "NXdata", 1);

    // Prepare the data chunk storage.
    size_t numPoints = ws->getNPoints();

    // Must have at least 1 point!
    if (numPoints == 0) numPoints = 1;
    MDE::prepareNexusData(file, numPoints);

    BoxController_sptr bc = ws->getBoxController();
    size_t maxBoxes = bc->getMaxId();

    // Prepare the vectors we will fill with data.

    // Box type (0=None, 1=MDBox, 2=MDGridBox
    std::vector<int> boxType(maxBoxes, 0);
    // Recursion depth
    std::vector<int> depth(maxBoxes, -1);
    // Start/end indices into the list of events
    std::vector<uint64_t> box_event_index(maxBoxes*2, 0);
    // Min/Max extents in each dimension
    std::vector<double> extents(maxBoxes*nd*2, 0);
    // Inverse of the volume of the cell
    std::vector<double> inverseVolume(maxBoxes, 0);
    // Box cached signal/error squared
    std::vector<double> box_signal_errorsquared(maxBoxes*2, 0);

    // Start/end children IDs
    std::vector<int> box_children(maxBoxes*2, 0);

    // The slab start for events, start at 0
    uint64_t start = 0;

    // Get a starting iterator
    MDBoxIterator<MDE,nd> it(ws->getBox(), 1000, false);

    Progress * prog = new Progress(this, 0, 0.9, maxBoxes);

    IMDBox<MDE,nd> * box;
    while (true)
    {
      box = it.getBox();
      size_t id = box->getId();
      if (id < maxBoxes)
      {
        // Various bits of data about the box
        depth[id] = int(box->getDepth());
        box_signal_errorsquared[id*2] = double(box->getSignal());
        box_signal_errorsquared[id*2+1] = double(box->getErrorSquared());
        inverseVolume[id] = box->getInverseVolume();

        for (size_t d=0; d<nd; d++)
        {
          size_t newIndex = (id+d)*2;
          extents[newIndex] = box->getExtents(d).min;
          extents[newIndex+1] = box->getExtents(d).max;
        }

        // The start/end children IDs
        size_t numChildren = box->getNumChildren();
        if (numChildren > 0)
        {
          // Make sure that all children are ordered. TODO: This might not be needed if the IDs are rigorously done
          size_t lastId = box->getChild(0)->getId();
          for (size_t i = 1; i < numChildren; i++)
          {
            if (box->getChild(i)->getId() != lastId+1)
              throw std::runtime_error("Non-sequential child ID encountered!");
            lastId = box->getChild(i)->getId();
          }

          box_children[id*2] = int(box->getChild(0)->getId());
          box_children[id*2+1] = int(box->getChild(numChildren-1)->getId());
          boxType[id] = 2;
        }
        else
          boxType[id] = 1;


        MDBox<MDE,nd> * mdbox = dynamic_cast<MDBox<MDE,nd> *>(box);
        if (mdbox)
        {
          const std::vector<MDE> & events = mdbox->getEvents();
          if (events.size() > 0)
          {
            mdbox->setFileIndex(uint64_t(start), uint64_t(events.size()));
            mdbox->saveNexus(file);
            // std::cout << id << " starts at " << start[0] << std::endl;
            // Save the index
            box_event_index[id*2] = start;
            box_event_index[id*2+1] = start + uint64_t(events.size());
            // Move forward in the file.
            start += uint64_t(events.size());
          }
        }

        // Move on to the next box
        prog->report();
        if (!it.next()) break;
      }
      else
      {
        // Some sort of problem
        g_log.warning() << "Unexpected box ID found (" << id << ") which is > than maxBoxes (" << maxBoxes << ")" << std::endl;
        break;
      }
    }

    // Done writing the event data
    MDE::closeNexusData(file);


//    for (size_t i=0; i<20; i++)
//    {
//      std::cout << i << " : " << box_event_index[i] << std::endl;
//    }

    // OK, we've filled these big arrays of data. Save them.
    prog->report("Writing Box Data");

    file->writeData("box_type", boxType);
    file->writeData("depth", depth);
    file->writeData("inverseVolume", inverseVolume);

    std::vector<int> dims(2,0);
    dims[0] = (int(maxBoxes));
    dims[1] = (nd*2);
    file->writeData("extents", extents, dims);

    dims[0] = (int(maxBoxes));
    dims[1] = (2);
    file->writeData("box_children", box_children, dims);

    file->writeData("box_signal_errorsquared", box_signal_errorsquared, dims);

    file->writeData("box_event_index", box_event_index, dims);

//    dims.clear();
//    file->writeData("children", children);

    file->close();

    delete prog;

  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveMDEW::exec()
  {
    IMDEventWorkspace_sptr ws = getProperty("InputWorkspace");

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->doSave, ws);
  }



} // namespace Mantid
} // namespace MDEvents

