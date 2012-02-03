#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Utils.h"
#include "MantidAPI/BoxController.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"


using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::Kernel::DateAndTime;
using Mantid::DataHandling::LoadInstrument;
using Mantid::DataObjects::EventWorkspace;
using Mantid::API::FrameworkManager;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{


/** Set of helper methods for testing MDEventWorkspace things
 *
 * @author Janik Zikovsky
 * @date March 29, 2011
 * */
namespace MDEventsTestHelper
{

  //-------------------------------------------------------------------------------------
  /** Create an EventWorkspace containing fake data
   * of single-crystal diffraction.
   * Instrument is MINITOPAZ
   *
   * @return EventWorkspace_sptr
   */
  EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents)
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "TEST");
    int numPixels = 10000;
    int numBins = 1600;
    double binDelta = 10.0;

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numPixels,1,1);

    // --------- Load the instrument -----------
    LoadInstrument * loadInst = new LoadInstrument();
    loadInst->initialize();
    loadInst->setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml");
    loadInst->setProperty<Mantid::API::MatrixWorkspace_sptr> ("Workspace", retVal);
    loadInst->execute();
    delete loadInst;
    // Populate the instrument parameters in this workspace - this works around a bug
    retVal->populateInstrumentParameters();

    DateAndTime run_start("2010-01-01");

    for (int pix = 0; pix < numPixels; pix++)
    {
      for (int i=0; i<numEvents; i++)
      {
        retVal->getEventList(pix) += Mantid::DataObjects::TofEvent((i+0.5)*binDelta, run_start+double(i));
      }
      retVal->getEventList(pix).addDetectorID(pix);
    }
    retVal->doneAddingEventLists();

    //Create the x-axis for histogramming.
    Mantid::MantidVecPtr x1;
    Mantid::MantidVec& xRef = x1.access();
    xRef.resize(numBins);
    for (int i = 0; i < numBins; ++i)
    {
      xRef[i] = i*binDelta;
    }

    //Set all the histograms at once.
    retVal->setAllX(x1);
    // Default unit: TOF.
    retVal->getAxis(0)->setUnit("TOF");

    // Give it a crystal and goniometer
    WorkspaceCreationHelper::SetGoniometer(retVal, 0., 0., 0.);
    WorkspaceCreationHelper::SetOrientedLattice(retVal, 1., 1., 1.);

    // Some sanity checks
    if (retVal->getInstrument()->getName() != "MINITOPAZ")
      throw std::runtime_error("MDEventsTestHelper::createDiffractionEventWorkspace(): Wrong instrument loaded.");
    Mantid::detid2det_map dets;
    retVal->getInstrument()->getDetectors(dets);
    if ( dets.size() != 100*100)
      throw std::runtime_error("MDEventsTestHelper::createDiffractionEventWorkspace(): Wrong instrument size.");


    return retVal;
  }



  //=====================================================================================
  /** Make a (optionally) file backed MDEventWorkspace with 10000 fake random data points
   *
   * @param wsName :: name of the workspace in ADS
   * @param fileBacked :: true for file-backed
   * @return MDEW sptr
   */
  MDEventWorkspace3Lean::sptr makeFileBackedMDEW(std::string wsName, bool fileBacked)
  {
    // Name of the output workspace.
    std::string outWSName("CloneMDWorkspaceTest_OutputWS");

    // ---------- Make a file-backed MDEventWorkspace -----------------------
    MDEventWorkspace3Lean::sptr ws1 = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    ws1->getBoxController()->setSplitThreshold(100);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName,
        boost::dynamic_pointer_cast< Mantid::API::IMDEventWorkspace>(ws1));
    FrameworkManager::Instance().exec("FakeMDEventData", 6,
        "InputWorkspace", wsName.c_str(),
        "UniformParams", "10000", "RandomizeSignal", "1");
    if (fileBacked)
    {
      std::string filename = wsName + ".nxs";
      Mantid::API::IAlgorithm_sptr saver = FrameworkManager::Instance().exec("SaveMD", 4,
          "InputWorkspace", wsName.c_str(),
          "Filename", filename.c_str());
      FrameworkManager::Instance().exec("LoadMD", 8,
          "OutputWorkspace", wsName.c_str(),
          "Filename", saver->getPropertyValue("Filename").c_str(),
          "FileBackEnd", "1", "Memory", "0");
    }
    return boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(  Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );
  }



  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox */
  MDBox<MDLeanEvent<1>,1> * makeMDBox1(size_t splitInto)
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(1));
    splitter->setSplitThreshold(5);
    // Splits into 10 boxes
    splitter->setSplitInto(splitInto);
    // Set the size
    MDBox<MDLeanEvent<1>,1> * out = new MDBox<MDLeanEvent<1>,1>(splitter);
    out->setExtents(0, 0.0, 10.0);
    out->calcVolume();
    return out;
  }

  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox with 3 dimensions, split 10x5x2 */
  MDBox<MDLeanEvent<3>,3> * makeMDBox3()
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(3));
    splitter->setSplitThreshold(5);
    // Splits into 10x5x2 boxes
    splitter->setSplitInto(10);
    splitter->setSplitInto(1,5);
    splitter->setSplitInto(2,2);
    // Set the size to 10.0 in all directions
    MDBox<MDLeanEvent<3>,3> * out = new MDBox<MDLeanEvent<3>,3>(splitter);
    for (size_t d=0; d<3; d++)
      out->setExtents(d, 0.0, 10.0);
    out->calcVolume();
    return out;
  }


  //-------------------------------------------------------------------------------------
  /** Return a vector with this many MDEvents, spaced evenly from 0.5, 1.5, etc. */
  std::vector<MDLeanEvent<1> > makeMDEvents1(size_t num)
  {
    std::vector<MDLeanEvent<1> > out;
    for (double i=0; i<num; i++)
    {
      coord_t coords[1] = {i*1.0+0.5};
      out.push_back( MDLeanEvent<1>(1.0, 1.0, coords) );
    }
    return out;
  }



  //-------------------------------------------------------------------------------------
  /** Creates a fake MDHistoWorkspace
   *
   * @param signal :: signal in every point
   * @param numDims :: number of dimensions to create. They will range from 0 to max
   * @param numBins :: bins in each dimensions
   * @param max :: max position in each dimension
   * @param errorSquared :: error squared in every point
   * @param name :: optional name
   * @return the MDHisto
   */
  Mantid::MDEvents::MDHistoWorkspace_sptr makeFakeMDHistoWorkspace(double signal, size_t numDims, size_t numBins,
      double max, double errorSquared, std::string name)
  {
    Mantid::MDEvents::MDHistoWorkspace * ws = NULL;
    if (numDims ==1)
    {
      ws = new Mantid::MDEvents::MDHistoWorkspace(
          MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)) );
    }
    else if (numDims == 2)
    {
      ws = new Mantid::MDEvents::MDHistoWorkspace(
          MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
          MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, max, numBins))  );
    }
    else if (numDims == 3)
    {
      ws = new Mantid::MDEvents::MDHistoWorkspace(
          MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
          MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, max, numBins)),
          MDHistoDimension_sptr(new MDHistoDimension("z","z","m", 0.0, max, numBins))   );
    }
    else if (numDims == 4)
    {
      ws = new Mantid::MDEvents::MDHistoWorkspace(
          MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
          MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, max, numBins)),
          MDHistoDimension_sptr(new MDHistoDimension("z","z","m", 0.0, max, numBins)),
          MDHistoDimension_sptr(new MDHistoDimension("t","z","m", 0.0, max, numBins))
          );
    }
    Mantid::MDEvents::MDHistoWorkspace_sptr ws_sptr(ws);
    ws_sptr->setTo(signal, errorSquared);
    if (!name.empty())
      AnalysisDataService::Instance().addOrReplace(name, ws_sptr);
    return ws_sptr;
  }



  //-------------------------------------------------------------------------------------
  /** Creates a fake MDHistoWorkspace with more options
   *
   * @param numDims :: number of dimensions to create. They will range from 0 to max
   * @param signal :: signal in every point
   * @param errorSquared :: error squared in every point
   * @param numBins :: array of # of bins in each dimensions
   * @param min :: array of min position in each dimension
   * @param max :: array of max position in each dimension
   * @param name :: optional name
   * @return the MDHisto
   */
  Mantid::MDEvents::MDHistoWorkspace_sptr makeFakeMDHistoWorkspaceGeneral(size_t numDims,
      double signal, double errorSquared,
      size_t * numBins, double * min, double * max,
      std::string name)
  {
    std::vector<std::string> names;
    names.push_back("x");
    names.push_back("y");
    names.push_back("z");
    names.push_back("t");

    std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
    for (size_t d=0; d<numDims; d++)
      dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(names[d], names[d], "m", min[d], max[d], numBins[d])));

    Mantid::MDEvents::MDHistoWorkspace * ws = NULL;
    ws = new Mantid::MDEvents::MDHistoWorkspace(dimensions);
    Mantid::MDEvents::MDHistoWorkspace_sptr ws_sptr(ws);
    ws_sptr->setTo(signal, errorSquared);
    if (!name.empty())
      AnalysisDataService::Instance().addOrReplace(name, ws_sptr);
    return ws_sptr;
  }


} // namespace
}
}
