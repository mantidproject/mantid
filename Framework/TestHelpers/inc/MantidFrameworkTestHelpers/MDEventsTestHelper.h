// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below the level
 *of
 *  DataObjects (e.g. Kernel, Geometry, API).
 *********************************************************************************/
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/WarningSuppressions.h"

namespace {
template <typename MDE, size_t nd>
std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> createOutputWorkspace(size_t splitInto) {
  std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> out(
      new Mantid::DataObjects::MDEventWorkspace<MDE, nd>());
  Mantid::API::BoxController_sptr bc = out->getBoxController();
  bc->setSplitThreshold(100);
  bc->setSplitInto(splitInto);
  return out;
}

template <typename MDE, size_t nd>
void addMDDimensions(std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> out, Mantid::coord_t min,
                     Mantid::coord_t max, const std::string &axisNameFormat, const std::string &axisIdFormat) {

  // Create MDFrame of General Frame type
  Mantid::Geometry::GeneralFrame frame(Mantid::Geometry::GeneralFrame::GeneralFrameDistance, "m");

  // Create dimensions
  for (size_t d = 0; d < nd; d++) {
    char name[200];
    sprintf(name, axisNameFormat.c_str(), d);
    char id[200];
    sprintf(id, axisIdFormat.c_str(), d);

    Mantid::Geometry::MDHistoDimension_sptr dim(
        new Mantid::Geometry::MDHistoDimension(std::string(name), std::string(id), frame, min, max, 10));
    out->addDimension(dim);
  }
  out->initialize();
}

template <typename MDE, size_t nd>
void addMDDimensionsWithFrames(std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> out, Mantid::coord_t min,
                               Mantid::coord_t max, const Mantid::Geometry::MDFrame &frame,
                               const std::string &axisNameFormat, const std::string &axisIdFormat) {
  for (size_t d = 0; d < nd; d++) {
    char name[200];
    sprintf(name, axisNameFormat.c_str(), d);
    char id[200];
    sprintf(id, axisIdFormat.c_str(), d);

    // Use the same frame for all dimensions
    auto dim =
        std::make_shared<Mantid::Geometry::MDHistoDimension>(std::string(name), std::string(id), frame, min, max, 10);
    out->addDimension(dim);
  }
  out->initialize();
}

template <typename MDE, size_t nd>
void addMDDimensionsWithIndividualFrames(std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> out,
                                         Mantid::coord_t min, Mantid::coord_t max,
                                         const std::vector<Mantid::Geometry::MDFrame_sptr> &frame,
                                         const std::string &axisNameFormat, const std::string &axisIdFormat) {
  for (size_t d = 0; d < nd; d++) {
    char name[200];
    sprintf(name, axisNameFormat.c_str(), d);
    char id[200];
    sprintf(id, axisIdFormat.c_str(), d);

    // Use the same frame for all dimensions
    auto dim = std::make_shared<Mantid::Geometry::MDHistoDimension>(std::string(name), std::string(id), *frame[d], min,
                                                                    max, 10);
    out->addDimension(dim);
  }
  out->initialize();
}

template <typename MDE, size_t nd>
void addData(std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> out, size_t splitInto, Mantid::coord_t min,
             Mantid::coord_t max, size_t numEventsPerBox) {
  if (numEventsPerBox > 0) {
    out->splitBox();
    size_t index[nd];
    Mantid::Kernel::Utils::NestedForLoop::SetUp(nd, index);
    size_t index_max[nd];
    Mantid::Kernel::Utils::NestedForLoop::SetUp(nd, index_max, splitInto);
    bool allDone = false;
    while (!allDone) {
      for (size_t i = 0; i < numEventsPerBox; i++) {
        // Put an event in the middle of each box
        Mantid::coord_t centers[nd];
        for (size_t d = 0; d < nd; d++)
          centers[d] = min + (static_cast<Mantid::coord_t>(index[d]) + 0.5f) * (max - min) /
                                 static_cast<Mantid::coord_t>(splitInto);
        out->addEvent(MDE(1.0, 1.0, centers));
      }

      allDone = Mantid::Kernel::Utils::NestedForLoop::Increment(nd, index, index_max);
    }
    out->refreshCache();
  }
  auto ei = Mantid::API::ExperimentInfo_sptr(new Mantid::API::ExperimentInfo());
  out->addExperimentInfo(ei);
}
} // namespace

namespace Mantid {
namespace DataObjects {

/** Set of helper methods for testing MDEventWorkspace things
 *
 * @author Janik Zikovsky
 * @date March 29, 2011
 * */
namespace MDEventsTestHelper {

/** Create an EventWorkspace containing fake data
 * of single-crystal diffraction.
 * Instrument is MINITOPAZ
 *
 * @return EventWorkspace_sptr
 */
Mantid::DataObjects::EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents, int numPixels = 400,
                                                                         int numBins = 160);

/// Make a (optionally) file backed MDEventWorkspace with 10000 fake random data points
MDEventWorkspace3Lean::sptr makeFakeMDEventWorkspace(const std::string &wsName, long numEvents = 10000,
                                                     Kernel::SpecialCoordinateSystem coord = Kernel::None);

/// Make a fake n-dimensional MDHistoWorkspace
MDHistoWorkspace_sptr makeFakeMDHistoWorkspace(double signal, size_t numDims, size_t numBins = 10, coord_t max = 10.0,
                                               double errorSquared = 1.0, const std::string &name = "",
                                               double numEvents = 1.0);

Mantid::DataObjects::MDHistoWorkspace_sptr
makeFakeMDHistoWorkspaceWithMDFrame(double signal, size_t numDims, const Mantid::Geometry::MDFrame &frame,
                                    size_t numBins = 10, coord_t max = 10.0, double errorSquared = 1.0,
                                    const std::string &name = "", double numEvents = 1.0);

/// More general fake n-dimensionsal MDHistoWorkspace
Mantid::DataObjects::MDHistoWorkspace_sptr makeFakeMDHistoWorkspaceGeneral(size_t numDims, double signal,
                                                                           double errorSquared, size_t *numBins,
                                                                           coord_t *min, coord_t *max,
                                                                           const std::string &name = "");
/// More general fake n-dimensionsal MDHistoWorkspace
Mantid::DataObjects::MDHistoWorkspace_sptr
makeFakeMDHistoWorkspaceGeneral(size_t numDims, double signal, double errorSquared, size_t *numBins, coord_t *min,
                                coord_t *max, std::vector<std::string> names, const std::string &name = "");

//-------------------------------------------------------------------------------------
/** Create a test MDEventWorkspace<nd> . Dimensions are names Axis0, Axis1, etc.
 *
 * @param splitInto :: each dimension will split into this many subgrids
 * @param min :: extent of each dimension (min)
 * @param max :: extent of each dimension (max)
 * @param numEventsPerBox :: will create one MDLeanEvent in the center of each
 *sub-box.
 *        0 = don't split box, don't add events
 * @param wsName :: if specified, then add the workspace to the analysis data
 *service
 * @param axisNameFormat :: string for the axis name, processed via sprintf()
 * @param axisIdFormat :: string for the axis ID, processed via sprintf()
 * @return shared ptr to the created workspace
 */
template <typename MDE, size_t nd>
std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>>
makeAnyMDEW(size_t splitInto, coord_t min, coord_t max, size_t numEventsPerBox = 0, const std::string &wsName = "",
            const std::string &axisNameFormat = "Axis%d", const std::string &axisIdFormat = "Axis%d") {
  // Create bare workspace
  auto out = createOutputWorkspace<MDE, nd>(splitInto);

  // Add standard dimensions
  addMDDimensions<MDE, nd>(out, min, max, axisNameFormat, axisIdFormat);

  // Add data
  addData<MDE, nd>(out, splitInto, min, max, numEventsPerBox);

  // Add to ADS on option
  if (!wsName.empty())
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName, out);

  return out;
}

/** Create a test MDEventWorkspace<nd> . Dimensions are names Axis0, Axis1, etc.
 *  But you can set an MDFrame. The frames can be set individually.
 *
 * @param splitInto :: each dimension will split into this many subgrids
 * @param min :: extent of each dimension (min)
 * @param max :: extent of each dimension (max)
 * @param frames:: the chosen frame
 * @param numEventsPerBox :: will create one MDLeanEvent in the center of each
 *sub-box.
 *        0 = don't split box, don't add events
 * @param wsName :: if specified, then add the workspace to the analysis data
 *service
 * @param axisNameFormat :: string for the axis name, processed via sprintf()
 * @param axisIdFormat :: string for the axis ID, processed via sprintf()
 * @return shared ptr to the created workspace
 */
template <typename MDE, size_t nd>
std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>>
makeAnyMDEWWithIndividualFrames(size_t splitInto, coord_t min, coord_t max,
                                std::vector<Mantid::Geometry::MDFrame_sptr> frames, size_t numEventsPerBox = 0,
                                const std::string &wsName = "", std::string axisNameFormat = "Axis%d",
                                std::string axisIdFormat = "Axis%d") {
  // Create bare workspace
  auto out = createOutputWorkspace<MDE, nd>(splitInto);

  // Add standard dimensions
  addMDDimensionsWithIndividualFrames<MDE, nd>(out, min, max, frames, axisNameFormat, axisIdFormat);

  // Add data
  addData<MDE, nd>(out, splitInto, min, max, numEventsPerBox);

  // Add to ADS on option
  if (!wsName.empty())
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName, out);

  return out;
}

/** Create a test MDEventWorkspace<nd> . Dimensions are names Axis0, Axis1, etc.
 *  But you can set an MDFrame. For now the same frame for all dimensions
 *  is used.
 *
 * @param splitInto :: each dimension will split into this many subgrids
 * @param min :: extent of each dimension (min)
 * @param max :: extent of each dimension (max)
 * @param frame:: the chosen frame
 * @param numEventsPerBox :: will create one MDLeanEvent in the center of each
 *sub-box.
 *        0 = don't split box, don't add events
 * @param wsName :: if specified, then add the workspace to the analysis data
 *service
 * @param axisNameFormat :: string for the axis name, processed via sprintf()
 * @param axisIdFormat :: string for the axis ID, processed via sprintf()
 * @return shared ptr to the created workspace
 */
template <typename MDE, size_t nd>
std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>>
makeAnyMDEWWithFrames(size_t splitInto, coord_t min, coord_t max, const Mantid::Geometry::MDFrame &frame,
                      size_t numEventsPerBox = 0, const std::string &wsName = "",
                      const std::string &axisNameFormat = "Axis%d", const std::string &axisIdFormat = "Axis%d") {
  // Create bare workspace
  auto out = createOutputWorkspace<MDE, nd>(splitInto);

  // Add standard dimensions
  addMDDimensionsWithFrames<MDE, nd>(out, min, max, frame, axisNameFormat, axisIdFormat);

  // Add data
  addData<MDE, nd>(out, splitInto, min, max, numEventsPerBox);

  // Add to ADS on option
  if (!wsName.empty())
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName, out);

  return out;
}

/** Make a MDEventWorkspace with MDLeanEvents */
template <size_t nd>
std::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>, nd>> makeMDEW(size_t splitInto, coord_t min, coord_t max,
                                                                size_t numEventsPerBox = 0) {
  return makeAnyMDEW<MDLeanEvent<nd>, nd>(splitInto, min, max, numEventsPerBox);
}

/** Make a MDEventWorkspace with MDLeanEvents nad MDFrames*/
template <size_t nd>
std::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>, nd>> makeMDEWWithFrames(size_t splitInto, coord_t min, coord_t max,
                                                                          const Mantid::Geometry::MDFrame &frame,
                                                                          size_t numEventsPerBox = 0) {
  return makeAnyMDEWWithFrames<MDLeanEvent<nd>, nd>(splitInto, min, max, frame, numEventsPerBox);
}

/** Make a MDEventWorkspace with MDLeanEvents and individual MDFrames*/
template <size_t nd>
std::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>, nd>>
makeMDEWWithIndividualFrames(size_t splitInto, coord_t min, coord_t max,
                             const std::vector<Mantid::Geometry::MDFrame_sptr> &frame, size_t numEventsPerBox = 0) {
  return makeAnyMDEWWithIndividualFrames<MDLeanEvent<nd>, nd>(splitInto, min, max, frame, numEventsPerBox);
}

/** Make a MDEventWorkspace with MDEvents  - updated to split dims by splitInto,
 * not 10 */
template <size_t nd>
std::shared_ptr<MDEventWorkspace<MDEvent<nd>, nd>> makeMDEWFull(size_t splitInto, coord_t min, coord_t max,
                                                                size_t numEventsPerBox = 0) {
  return makeAnyMDEW<MDEvent<nd>, nd>(splitInto, min, max, numEventsPerBox);
}

//=====================================================================================
//=============================== MDGRIDBOX HELPER METHODS
//============================
//=====================================================================================

/** Generate an empty MDBox */
MDBox<MDLeanEvent<1>, 1> *makeMDBox1(size_t splitInto = 10, API::BoxController *splitter = nullptr);

/** Generate an empty MDBox with 3 dimensions, split 10x5x2 */
MDBox<MDLeanEvent<3>, 3> *makeMDBox3();

/** Return a vector with this many MDEvents, spaced evenly from 0.5, 1.5, etc.
 */
std::vector<MDLeanEvent<1>> makeMDEvents1(size_t num);

// The makeMDGridBox method makes a unique_ptr to an MDBox, passes the raw pointer to
// another MBBox, then as the unique_ptr goes out of scope the first MDBox will be
// deleted, potentially leaving a dangling pointer. However it seems to work, and it's
// only used in tests. Would be good to fix this though.
GNU_DIAG_OFF("array-bounds")

//-------------------------------------------------------------------------------------
/** Generate an empty MDBox with 2 dimensions, splitting in (default) 10x10
 *boxes.
 * Box size is 10x10.
 *
 * @param split0 :: for uneven splitting
 * @param split1 :: for uneven splitting
 * @param dimensionMin :: minimum dimesion extent
 * @param dimensionMax :: maximum dimesion extent
 */
template <size_t nd>
static MDGridBox<MDLeanEvent<nd>, nd> *makeMDGridBox(size_t split0 = 10, size_t split1 = 10, coord_t dimensionMin = 0.0,
                                                     coord_t dimensionMax = 10.0) {
  // Split at 5 events
  auto splitter = new Mantid::API::BoxController(nd);
  splitter->setSplitThreshold(5);
  // Splits into 10x10x.. boxes
  splitter->setSplitInto(split0);
  splitter->setSplitInto(0, split0);
  if (nd > 1)
    splitter->setSplitInto(1, split1);
  // Set the size to 10.0 in all directions
  auto box = std::make_unique<MDBox<MDLeanEvent<nd>, nd>>(splitter);
  for (size_t d = 0; d < nd; d++)
    // carefull! function with the side effects!
    box->setExtents(d, dimensionMin, dimensionMax);
  // calc volume necessary
  box->calcVolume();

  // Split
  auto out = new MDGridBox<MDLeanEvent<nd>, nd>(box.get());
  return out;
}

GNU_DIAG_ON("array-bounds")

//-------------------------------------------------------------------------------------
/** Feed a MDGridBox with evenly-spaced events
 *
 * @param box :: MDGridBox pointer
 * @param repeat :: how many events to stick in the same place
 * @param numPerSide :: e.g. if 10, and 3 dimensions, there will be 10x10x10
 *events
 * @param start :: x-coordinate starts at this for event 0
 * @param step :: x-coordinate increases by this much.
 */
template <size_t nd>
static void feedMDBox(MDBoxBase<MDLeanEvent<nd>, nd> *box, size_t repeat = 1, size_t numPerSide = 10,
                      coord_t start = 0.5, coord_t step = 1.0) {
  size_t counters[nd];
  Mantid::Kernel::Utils::NestedForLoop::SetUp(nd, counters, 0);
  size_t index_max[nd];
  Mantid::Kernel::Utils::NestedForLoop::SetUp(nd, index_max, numPerSide);
  // Recursive for loop
  bool allDone = false;
  while (!allDone) {
    // Generate the position from the counter
    coord_t centers[nd];
    for (size_t d = 0; d < nd; d++)
      centers[d] = static_cast<coord_t>(counters[d]) * step + start;

    // Add that event 'repeat' times
    for (size_t i = 0; i < repeat; ++i)
      box->addEvent(MDLeanEvent<nd>(1.0, 1.0, centers));

    // Increment the nested for loop
    allDone = Mantid::Kernel::Utils::NestedForLoop::Increment(nd, counters, index_max);
  }
  box->refreshCache(nullptr);
}

//-------------------------------------------------------------------------------------
/** Recursively split an existing MDGridBox
 *
 * @param box :: box to split
 * @param atRecurseLevel :: This is the recursion level at which we are
 * @param recurseLimit :: this is where to spot
 */
template <size_t nd>
static void recurseSplit(MDGridBox<MDLeanEvent<nd>, nd> *box, size_t atRecurseLevel, size_t recurseLimit) {
  using boxVector = std::vector<MDBoxBase<MDLeanEvent<nd>, nd> *>;
  if (atRecurseLevel >= recurseLimit)
    return;

  // Split all the contents
  boxVector boxes;
  boxes = box->getBoxes();
  for (size_t i = 0; i < boxes.size(); i++)
    box->splitContents(i);

  // Retrieve the contained MDGridBoxes
  boxes = box->getBoxes();

  // Go through them and split them
  for (size_t i = 0; i < boxes.size(); i++) {
    MDGridBox<MDLeanEvent<nd>, nd> *containedbox = dynamic_cast<MDGridBox<MDLeanEvent<nd>, nd> *>(boxes[i]);
    if (containedbox)
      recurseSplit(containedbox, atRecurseLevel + 1, recurseLimit);
  }
}

//-------------------------------------------------------------------------------------
/** Generate a recursively gridded MDGridBox
 *
 * @param splitInto :: boxes split into this many boxes/side
 * @param levels :: levels of splitting recursion (0=just the top level is
 *split)
 * @return
 */
template <size_t nd> static MDGridBox<MDLeanEvent<nd>, nd> *makeRecursiveMDGridBox(size_t splitInto, size_t levels) {
  // Split at 5 events
  auto splitter(new Mantid::API::BoxController(nd));
  splitter->setSplitThreshold(5);
  splitter->resetNumBoxes();
  splitter->setMaxDepth(levels + 1);
  // Splits into splitInto x splitInto x ... boxes
  splitter->setSplitInto(splitInto);
  // Set the size to splitInto*1.0 in all directions
  auto box = std::make_unique<MDBox<MDLeanEvent<nd>, nd>>(splitter);
  for (size_t d = 0; d < nd; d++)
    box->setExtents(d, 0.0, static_cast<coord_t>(splitInto));
  // Split into the gridbox.
  auto gridbox = new MDGridBox<MDLeanEvent<nd>, nd>(box.get());
  // Now recursively split more
  recurseSplit(gridbox, 0, levels);

  return gridbox;
}

//-------------------------------------------------------------------------------------
/** Helper function compares the extents of the given box */
template <typename MDBOX> static void extents_match(MDBOX box, size_t dim, double min, double max) {
  TSM_ASSERT_DELTA(dim, box->getExtents(dim).getMin(), min, 1e-6);
  TSM_ASSERT_DELTA(dim, box->getExtents(dim).getMax(), max, 1e-6);
}

void checkAndDeleteFile(const std::string &filename);

//=====================================================================================
//===================================== TEST METHODS
//==================================
//=====================================================================================

} // namespace MDEventsTestHelper
} // namespace DataObjects
} // namespace Mantid
