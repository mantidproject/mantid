// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This collection of functions MAY NOT be used in any test from a package
 *below
 *  the level of MDEvents (e.g. Kernel, Geometry, API, DataObjects).
 *********************************************************************************/
#include "MantidAPI/Axis.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/FakeMD.h"
#include "MantidDataObjects/MDEventWorkspace.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/cow_ptr.h"

#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/make_shared.hpp>

#include <Poco/File.h>

namespace Mantid {
namespace DataObjects {

using namespace Mantid::API;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::Geometry::InstrumentDefinitionParser;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Types::Core::DateAndTime;
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
  double binDelta = 10.0;

  auto retVal = boost::make_shared<EventWorkspace>();
  retVal->initialize(numPixels, 1, 1);

  // --------- Load the instrument -----------
  const std::string filename = FileFinder::Instance().getFullPath(
      "unit_testing/MINITOPAZ_Definition.xml");
  InstrumentDefinitionParser parser(filename, "MINITOPAZ",
                                    Strings::loadFile(filename));
  auto instrument = parser.parseXML(nullptr);
  retVal->populateInstrumentParameters();
  retVal->setInstrument(instrument);

  DateAndTime run_start("2010-01-01T00:00:00");

  for (int pix = 0; pix < numPixels; pix++) {
    for (int i = 0; i < numEvents; i++) {
      retVal->getSpectrum(pix) +=
          Types::Event::TofEvent((i + 0.5) * binDelta, run_start + double(i));
    }
    retVal->getSpectrum(pix).addDetectorID(pix);
  }

  // Create the x-axis for histogramming.
  HistogramData::BinEdges x1(numBins);
  auto &xRef = x1.mutableData();
  for (int i = 0; i < numBins; ++i) {
    xRef[i] = i * binDelta;
  }

  // Set all the histograms at once.
  retVal->setAllX(x1);
  // Default unit: TOF.
  retVal->getAxis(0)->setUnit("TOF");

  // Give it a crystal and goniometer
  WorkspaceCreationHelper::setGoniometer(retVal, 0., 0., 0.);
  WorkspaceCreationHelper::setOrientedLattice(retVal, 1., 1., 1.);

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
/** Make an MDEventWorkspace with nEvents fake data
 *points
 * the points are randomly distributed within the box (nEvents>0) or
 *homoheneously and regularly spread through the box (nEvents<0)
 *
 * @param wsName :: name of the workspace in ADS
 * @param numEvents :: number of events in the target workspace distributed
 *randomly if numEvents>0 or regularly & homogeneously if numEvents<0
 * @param coord :: Required coordinate system
 * @return MDEW sptr
 */
MDEventWorkspace3Lean::sptr
makeFakeMDEventWorkspace(const std::string &wsName, long numEvents,
                         Kernel::SpecialCoordinateSystem coord) {
  // ---------- Make a file-backed MDEventWorkspace -----------------------
  MDEventWorkspace3Lean::sptr ws1 =
      MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
  ws1->setCoordinateSystem(coord);
  ws1->getBoxController()->setSplitThreshold(100);
  API::AnalysisDataService::Instance().addOrReplace(
      wsName, boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws1));
  FakeMD dataFaker(std::vector<double>(1, static_cast<double>(numEvents)),
                   std::vector<double>(), 0, true);
  dataFaker.fill(ws1);
  return boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(
      API::AnalysisDataService::Instance().retrieve(wsName));
}

//-------------------------------------------------------------------------------------
/** Generate an empty MDBox ,
!!! Box controller has to be deleted saparately to avoid memory leaks in tests
!!!!*/
MDBox<MDLeanEvent<1>, 1> *makeMDBox1(size_t splitInto,
                                     BoxController *splitter) {
  if (!splitter) {
    splitter = (new BoxController(1));
  }
  // Split at 5 events
  splitter->setSplitThreshold(5);
  // Splits into 10 boxes
  splitter->setSplitInto(splitInto);
  // Set the size
  auto *out = new MDBox<MDLeanEvent<1>, 1>(splitter);
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
  auto splitter = new BoxController(3);
  splitter->setSplitThreshold(5);
  // Splits into 10x5x2 boxes
  splitter->setSplitInto(10);
  splitter->setSplitInto(1, 5);
  splitter->setSplitInto(2, 2);
  // Set the size to 10.0 in all directions
  auto out = new MDBox<MDLeanEvent<3>, 3>(splitter);
  for (size_t d = 0; d < 3; ++d) {
    out->setExtents(d, 0.0, 10.0);
  }
  out->calcVolume();
  return out;
}

//-------------------------------------------------------------------------------------
/** Return a vector with this many MDEvents, spaced evenly from 0.5, 1.5, etc.
 */
std::vector<MDLeanEvent<1>> makeMDEvents1(size_t num) {
  std::vector<MDLeanEvent<1>> out;
  out.reserve(num);
  for (std::size_t i = 0; i < num; ++i) {
    float coords[1] = {static_cast<float>(i) + 0.5f};
    out.emplace_back(1.0f, 1.0f, coords);
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
                         coord_t max, double errorSquared,
                         const std::string &name, double numEvents) {
  // Create MDFrame of General Frame type
  Mantid::Geometry::GeneralFrame frame(
      Mantid::Geometry::GeneralFrame::GeneralFrameDistance, "m");
  return makeFakeMDHistoWorkspaceWithMDFrame(
      signal, numDims, frame, numBins, max, errorSquared, name, numEvents);
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
  std::vector<std::string> names{"x", "y", "z", "t"};
  // Create MDFrame of General Frame type
  Mantid::Geometry::GeneralFrame frame(
      Mantid::Geometry::GeneralFrame::GeneralFrameDistance, "m");

  std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
  for (size_t d = 0; d < numDims; d++)
    dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(
        names[d], names[d], frame, min[d], max[d], numBins[d])));

  MDHistoWorkspace_sptr ws_sptr =
      boost::make_shared<MDHistoWorkspace>(dimensions);
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
  // Create MDFrame of General Frame type
  Mantid::Geometry::GeneralFrame frame(
      Mantid::Geometry::GeneralFrame::GeneralFrameDistance, "m");
  for (size_t d = 0; d < numDims; d++)
    dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(
        names[d], names[d], frame, min[d], max[d], numBins[d])));

  MDHistoWorkspace_sptr ws_sptr =
      boost::make_shared<MDHistoWorkspace>(dimensions);
  ws_sptr->setTo(signal, errorSquared, 1.0 /* num events */);
  if (!name.empty())
    AnalysisDataService::Instance().addOrReplace(name, ws_sptr);
  return ws_sptr;
}

//-------------------------------------------------------------------------------------
/** Creates a fake MDHistoWorkspace with MDFrame selection
 *
 * @param signal :: signal in every point
 * @param numDims :: number of dimensions to create. They will range from 0 to
 *max
 * @param frame :: the selected frame
 * @param numBins :: bins in each dimensions
 * @param max :: max position in each dimension
 * @param errorSquared :: error squared in every point
 * @param name :: optional name
 * @param numEvents :: optional number of events in each bin. Default 1.0
 * @return the MDHisto
 */
Mantid::DataObjects::MDHistoWorkspace_sptr makeFakeMDHistoWorkspaceWithMDFrame(
    double signal, size_t numDims, const Mantid::Geometry::MDFrame &frame,
    size_t numBins, coord_t max, double errorSquared, std::string name,
    double numEvents) {
  // MDHistoWorkspace *ws = nullptr;
  MDHistoWorkspace_sptr ws_sptr;
  if (numDims == 1) {
    ws_sptr = boost::make_shared<MDHistoWorkspace>(
        boost::make_shared<MDHistoDimension>("x", "x", frame, 0.0f, max,
                                             numBins));
  } else if (numDims == 2) {
    ws_sptr = boost::make_shared<MDHistoWorkspace>(
        boost::make_shared<MDHistoDimension>("x", "x", frame, 0.0f, max,
                                             numBins),
        boost::make_shared<MDHistoDimension>("y", "y", frame, 0.0f, max,
                                             numBins));
  } else if (numDims == 3) {
    ws_sptr = boost::make_shared<MDHistoWorkspace>(
        boost::make_shared<MDHistoDimension>("x", "x", frame, 0.0f, max,
                                             numBins),
        boost::make_shared<MDHistoDimension>("y", "y", frame, 0.0f, max,
                                             numBins),
        boost::make_shared<MDHistoDimension>("z", "z", frame, 0.0f, max,
                                             numBins));
  } else if (numDims == 4) {
    ws_sptr = boost::make_shared<MDHistoWorkspace>(
        boost::make_shared<MDHistoDimension>("x", "x", frame, 0.0f, max,
                                             numBins),
        boost::make_shared<MDHistoDimension>("y", "y", frame, 0.0f, max,
                                             numBins),
        boost::make_shared<MDHistoDimension>("z", "z", frame, 0.0f, max,
                                             numBins),
        boost::make_shared<MDHistoDimension>("t", "t", frame, 0.0f, max,
                                             numBins));
  }

  if (!ws_sptr)
    throw std::runtime_error(
        " invalid or unsupported number of dimensions given");

  ws_sptr->setTo(signal, errorSquared, numEvents);
  ws_sptr->addExperimentInfo(boost::make_shared<ExperimentInfo>());
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

} // namespace MDEventsTestHelper
} // namespace DataObjects
} // namespace Mantid
