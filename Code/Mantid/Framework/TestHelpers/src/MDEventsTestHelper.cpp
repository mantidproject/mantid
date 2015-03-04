/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This collection of functions MAY NOT be used in any test from a package
 *below
 *  the level of MDEvents (e.g. Kernel, Geometry, API, DataObjects).
 *********************************************************************************/
#include "MantidAPI/BoxController.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Utils.h"

#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/make_shared.hpp>

#include <Poco/File.h>

namespace Mantid {
namespace DataObjects {

using namespace Mantid::API;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::EventWorkspace;
using Mantid::Geometry::InstrumentDefinitionParser;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Kernel::DateAndTime;
namespace Strings = Mantid::Kernel::Strings;

/** Set of helper methods for testing MDEventWorkspace things
 *
 * @author Janik Zikovsky
 * @date March 29, 2011
 * */
namespace MDEventsTestHelper {

//-------------------------------------------------------------------------------------
/** Create an EventWorkspace containing fake data
 * of single-crystal diffraction.
 * Instrument is MINITOPAZ
 *
 * @return EventWorkspace_sptr
 */
EventWorkspace_sptr
createDiffractionEventWorkspace(int numEvents, int numPixels, int numBins) {
  FacilityHelper::ScopedFacilities loadTESTFacility(
      "IDFs_for_UNIT_TESTING/UnitTestFacilities.xml", "TEST");

  double binDelta = 10.0;

  EventWorkspace_sptr retVal(new EventWorkspace);
  retVal->initialize(numPixels, 1, 1);

  // --------- Load the instrument -----------
  const std::string filename = "IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml";
  InstrumentDefinitionParser parser;
  parser.initialize(filename, "MINITOPAZ", Strings::loadFile(filename));
  auto instrument = parser.parseXML(NULL);
  retVal->populateInstrumentParameters();
  retVal->setInstrument(instrument);

  DateAndTime run_start("2010-01-01T00:00:00");

  for (int pix = 0; pix < numPixels; pix++) {
    for (int i = 0; i < numEvents; i++) {
      retVal->getEventList(pix) += Mantid::DataObjects::TofEvent(
          (i + 0.5) * binDelta, run_start + double(i));
    }
    retVal->getEventList(pix).addDetectorID(pix);
  }

  // Create the x-axis for histogramming.
  Mantid::MantidVecPtr x1;
  Mantid::MantidVec &xRef = x1.access();
  xRef.resize(numBins);
  for (int i = 0; i < numBins; ++i) {
    xRef[i] = i * binDelta;
  }

  // Set all the histograms at once.
  retVal->setAllX(x1);
  // Default unit: TOF.
  retVal->getAxis(0)->setUnit("TOF");

  // Give it a crystal and goniometer
  WorkspaceCreationHelper::SetGoniometer(retVal, 0., 0., 0.);
  WorkspaceCreationHelper::SetOrientedLattice(retVal, 1., 1., 1.);

  // Some sanity checks
  if (retVal->getInstrument()->getName() != "MINITOPAZ")
    throw std::runtime_error("MDEventsTestHelper::"
                             "createDiffractionEventWorkspace(): Wrong "
                             "instrument loaded.");
  Mantid::detid2det_map dets;
  retVal->getInstrument()->getDetectors(dets);
  if (dets.size() != 100 * 100)
    throw std::runtime_error("MDEventsTestHelper::"
                             "createDiffractionEventWorkspace(): Wrong "
                             "instrument size.");

  return retVal;
}

//=====================================================================================
/** Make a (optionally) file backed MDEventWorkspace with nEvents fake data
 *points
 * the points are randomly distributed within the box (nEvents>0) or
 *homoheneously and regularly spread through the box (nEvents<0)
 *
 * @param wsName :: name of the workspace in ADS
 * @param fileBacked :: true for file-backed
 * @param numEvents :: number of events in the target workspace distributed
 *randomly if numEvents>0 or regularly & homogeneously if numEvents<0
 * @return MDEW sptr
 */
MDEventWorkspace3Lean::sptr
makeFakeMDEventWorkspace(const std::string & wsName, long numEvents) {
  // ---------- Make a file-backed MDEventWorkspace -----------------------
  std::string snEvents = boost::lexical_cast<std::string>(numEvents);
  MDEventWorkspace3Lean::sptr ws1 =
      MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
  ws1->getBoxController()->setSplitThreshold(100);
  API::AnalysisDataService::Instance().addOrReplace(
      wsName, boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws1));
  FrameworkManager::Instance().exec("FakeMDEventData", 6, "InputWorkspace",
                                    wsName.c_str(), "UniformParams",
                                    snEvents.c_str(), "RandomizeSignal", "1");
  return boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(
      API::AnalysisDataService::Instance().retrieve(wsName));
}

//-------------------------------------------------------------------------------------
/** Generate an empty MDBox ,
!!! Box controller has to be deleted saparately to avoid memory leaks in tests
!!!!*/
MDBox<MDLeanEvent<1>, 1> *makeMDBox1(size_t splitInto,
                                     BoxController *splitter) {
  if (!splitter)
    splitter = (new BoxController(1));

  // Split at 5 events
  splitter->setSplitThreshold(5);
  // Splits into 10 boxes
  splitter->setSplitInto(splitInto);
  // Set the size
  MDBox<MDLeanEvent<1>, 1> *out = new MDBox<MDLeanEvent<1>, 1>(splitter);
  out->setExtents(0, 0.0, 10.0);
  out->calcVolume();
  return out;
}

//-------------------------------------------------------------------------------------
/** Generate an empty MDBox with 3 dimensions, split 10x5x2
   !!! Box controller has to be deleted saparately to avoid memory leaks in
   tests !!!!**/
MDBox<MDLeanEvent<3>, 3> *makeMDBox3() {
  // Split at 5 events

  BoxController *splitter = new BoxController(3);

  splitter->setSplitThreshold(5);
  // Splits into 10x5x2 boxes
  splitter->setSplitInto(10);
  splitter->setSplitInto(1, 5);
  splitter->setSplitInto(2, 2);
  // Set the size to 10.0 in all directions
  MDBox<MDLeanEvent<3>, 3> *out = new MDBox<MDLeanEvent<3>, 3>(splitter);
  for (size_t d = 0; d < 3; d++)
    out->setExtents(d, 0.0, 10.0);
  out->calcVolume();
  return out;
}

//-------------------------------------------------------------------------------------
/** Return a vector with this many MDEvents, spaced evenly from 0.5, 1.5, etc.
 */
std::vector<MDLeanEvent<1>> makeMDEvents1(size_t num) {
  std::vector<MDLeanEvent<1>> out;
  for (double i = 0; i < num; i++) {
    double coords[1] = {i * 1.0 + 0.5};
    out.push_back(MDLeanEvent<1>(1.0, 1.0, coords));
  }
  return out;
}

//-------------------------------------------------------------------------------------
/** Creates a fake MDHistoWorkspace
 *
 * @param signal :: signal in every point
 * @param numDims :: number of dimensions to create. They will range from 0 to
 *max
 * @param numBins :: bins in each dimensions
 * @param max :: max position in each dimension
 * @param errorSquared :: error squared in every point
 * @param name :: optional name
 * @param numEvents :: optional number of events in each bin. Default 1.0
 * @return the MDHisto
 */
Mantid::DataObjects::MDHistoWorkspace_sptr
makeFakeMDHistoWorkspace(double signal, size_t numDims, size_t numBins,
                         coord_t max, double errorSquared, std::string name,
                         double numEvents) {
  MDHistoWorkspace *ws = NULL;
  if (numDims == 1) {
    ws = new MDHistoWorkspace(MDHistoDimension_sptr(
        new MDHistoDimension("x", "x", "m", 0.0, max, numBins)));
  } else if (numDims == 2) {
    ws = new MDHistoWorkspace(
        MDHistoDimension_sptr(
            new MDHistoDimension("x", "x", "m", 0.0, max, numBins)),
        MDHistoDimension_sptr(
            new MDHistoDimension("y", "y", "m", 0.0, max, numBins)));
  } else if (numDims == 3) {
    ws = new MDHistoWorkspace(
        MDHistoDimension_sptr(
            new MDHistoDimension("x", "x", "m", 0.0, max, numBins)),
        MDHistoDimension_sptr(
            new MDHistoDimension("y", "y", "m", 0.0, max, numBins)),
        MDHistoDimension_sptr(
            new MDHistoDimension("z", "z", "m", 0.0, max, numBins)));
  } else if (numDims == 4) {
    ws = new MDHistoWorkspace(
        MDHistoDimension_sptr(
            new MDHistoDimension("x", "x", "m", 0.0, max, numBins)),
        MDHistoDimension_sptr(
            new MDHistoDimension("y", "y", "m", 0.0, max, numBins)),
        MDHistoDimension_sptr(
            new MDHistoDimension("z", "z", "m", 0.0, max, numBins)),
        MDHistoDimension_sptr(
            new MDHistoDimension("t", "t", "m", 0.0, max, numBins)));
  }

  if (!ws)
    throw std::runtime_error(
        " invalid or unsupported number of dimensions given");

  MDHistoWorkspace_sptr ws_sptr(ws);
  ws_sptr->setTo(signal, errorSquared, numEvents);
  ws_sptr->addExperimentInfo(ExperimentInfo_sptr(new ExperimentInfo()));
  if (!name.empty())
    AnalysisDataService::Instance().addOrReplace(name, ws_sptr);
  return ws_sptr;
}

//-------------------------------------------------------------------------------------
/** Creates a fake MDHistoWorkspace with more options
 *
 * @param numDims :: number of dimensions to create. They will range from 0 to
 *max
 * @param signal :: signal in every point
 * @param errorSquared :: error squared in every point
 * @param numBins :: array of # of bins in each dimensions
 * @param min :: array of min position in each dimension
 * @param max :: array of max position in each dimension
 * @param name :: optional name
 * @return the MDHisto
 */
MDHistoWorkspace_sptr
makeFakeMDHistoWorkspaceGeneral(size_t numDims, double signal,
                                double errorSquared, size_t *numBins,
                                coord_t *min, coord_t *max, std::string name) {
  std::vector<std::string> names;
  names.push_back("x");
  names.push_back("y");
  names.push_back("z");
  names.push_back("t");

  std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
  for (size_t d = 0; d < numDims; d++)
    dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(
        names[d], names[d], "m", min[d], max[d], numBins[d])));

  MDHistoWorkspace *ws = NULL;
  ws = new MDHistoWorkspace(dimensions);
  MDHistoWorkspace_sptr ws_sptr(ws);
  ws_sptr->setTo(signal, errorSquared, 1.0 /* num events */);
  if (!name.empty())
    AnalysisDataService::Instance().addOrReplace(name, ws_sptr);
  return ws_sptr;
}

//-------------------------------------------------------------------------------------
/** Creates a fake MDHistoWorkspace with more options
 *
 * @param numDims :: number of dimensions to create. They will range from 0 to
 *max
 * @param signal :: signal in every point
 * @param errorSquared :: error squared in every point
 * @param numBins :: array of # of bins in each dimensions
 * @param min :: array of min position in each dimension
 * @param max :: array of max position in each dimension
 * @param names :: array of names for each dimension
 * @param name :: optional name
 * @return the MDHisto
 */
MDHistoWorkspace_sptr makeFakeMDHistoWorkspaceGeneral(
    size_t numDims, double signal, double errorSquared, size_t *numBins,
    coord_t *min, coord_t *max, std::vector<std::string> names,
    std::string name) {
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
  for (size_t d = 0; d < numDims; d++)
    dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(
        names[d], names[d], "m", min[d], max[d], numBins[d])));

  MDHistoWorkspace *ws = NULL;
  ws = new MDHistoWorkspace(dimensions);
  MDHistoWorkspace_sptr ws_sptr(ws);
  ws_sptr->setTo(signal, errorSquared, 1.0 /* num events */);
  if (!name.empty())
    AnalysisDataService::Instance().addOrReplace(name, ws_sptr);
  return ws_sptr;
}

/**
 * Delete a file from disk
 * @param filename : File name to check and delete
 */
void checkAndDeleteFile(std::string filename) {
  if (!filename.empty()) {
    if (Poco::File(filename).exists()) {
      Poco::File(filename).remove();
    }
  }
}

} // namespace
}
}
