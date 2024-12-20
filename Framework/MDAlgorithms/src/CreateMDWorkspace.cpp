// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Memory.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <cmath>

namespace Mantid::MDAlgorithms {
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using boost::regex;

/*
 * The list of dimension names often looks like "[H,0,0],[0,K,0]" with "[H,0,0]"
 * being the first dimension but getProperty returns a vector of
 * the string split on every comma
 * This function parses the string, and does not split on commas within brackets
 */
std::vector<std::string> parseNames(const std::string &names_string) {

  // This regex has two parts which are separated by the "|" (or)
  // The first part matches anything which is bounded by square brackets
  // unless they contain square brackets (so that it only matches inner pairs)
  // The second part matches anything that doesn't contain a comma
  // NB, the order of the two parts matters

  regex expression(R"(\[([^\[]*)\]|[^,]+)");

  boost::sregex_token_iterator iter(names_string.begin(), names_string.end(), expression, 0);
  boost::sregex_token_iterator end;

  std::vector<std::string> names_result(iter, end);

  return names_result;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateMDWorkspace)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMDWorkspace::init() {
  declareProperty(std::make_unique<PropertyWithValue<int>>("Dimensions", 1, Direction::Input),
                  "Number of dimensions that the workspace will have.");

  std::vector<std::string> propOptions{"MDEvent", "MDLeanEvent"};
  declareProperty("EventType", "MDLeanEvent", std::make_shared<StringListValidator>(propOptions),
                  "Which underlying data type will event take.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Extents"),
                  "A comma separated list of min, max for each dimension,\n"
                  "specifying the extents of each dimension.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("Names", Direction::Input),
                  "A comma separated list of the name of each dimension.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("Units"),
                  "A comma separated list of the units of each dimension.");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("Frames"),
                  " A comma separated list of the frames of each dimension. "
                  " The frames can be"
                  " **General Frame**: Any frame which is not a Q-based frame."
                  " **QLab**: Wave-vector converted into the lab frame."
                  " **QSample**: Wave-vector converted into the frame of the sample."
                  " **HKL**: Wave-vector converted into the crystal's HKL indices."
                  " Note if nothing is specified then the **General Frame** is being "
                  "selected. Also note that if you select a frame then this might override "
                  "your unit selection if it is not compatible with the frame.");
  // Set the box controller properties
  this->initBoxControllerProps("5", 1000, 5);

  declareProperty(std::make_unique<PropertyWithValue<int>>("MinRecursionDepth", 0),
                  "Optional. If specified, then all the boxes will be split to "
                  "this minimum recursion depth. 0 = no splitting, 1 = one "
                  "level of splitting, etc.\n"
                  "Be careful using this since it can quickly create a huge "
                  "number of boxes = (SplitInto ^ (MinRercursionDepth * "
                  "NumDimensions)).");
  setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace.");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::OptionalSave, ".nxs"),
                  "Optional: to use a file as the back end, give the path to the file to "
                  "save.");

  declareProperty(std::make_unique<PropertyWithValue<int>>("Memory", -1),
                  "If Filename is specified to use a file back end:\n"
                  "  The amount of memory (in MB) to allocate to the in-memory cache.\n"
                  "  If not specified, a default of 40% of free physical memory is used.");
  setPropertySettings("Memory", std::make_unique<EnabledWhenProperty>("Filename", IS_NOT_DEFAULT));
}

/** Finish initialisation
 *
 * @param ws :: MDEventWorkspace to finish
 */
template <typename MDE, size_t nd> void CreateMDWorkspace::finish(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  // ------------ Set up the box controller ----------------------------------
  BoxController_sptr bc = ws->getBoxController();
  this->setBoxController(bc);

  // Split to level 1
  ws->splitBox();

  // Do we split more due to MinRecursionDepth?
  int minDepth = this->getProperty("MinRecursionDepth");
  if (minDepth < 0)
    throw std::invalid_argument("MinRecursionDepth must be >= 0.");
  ws->setMinRecursionDepth(size_t(minDepth));
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMDWorkspace::exec() {
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

  auto ndims = static_cast<size_t>(ndims_prop);

  std::vector<double> extents = getProperty("Extents");
  std::string dimensions_string = getPropertyValue("Names");
  std::vector<std::string> names = parseNames(dimensions_string);

  std::vector<std::string> units = getProperty("Units");
  std::vector<std::string> frames = getProperty("Frames");

  if (extents.size() != ndims * 2)
    throw std::invalid_argument("You must specify twice as many extents "
                                "(min,max) as there are dimensions.");
  if (names.size() != ndims)
    throw std::invalid_argument("You must specify as many names as there are dimensions.");
  if (units.size() != ndims)
    throw std::invalid_argument("You must specify as many units as there are dimensions.");
  // If no frames are specified we want to default to the General Frame,
  // to ensure backward compatibility. But if they are only partly specified,
  // then we want to throw an error. It should be either used correctly or not
  // at all
  if (!frames.empty() && frames.size() != ndims) {
    throw std::invalid_argument("You must specify as many frames as there are dimensions.");
  }

  if (frames.empty()) {
    frames.resize(ndims);
    std::fill(frames.begin(), frames.end(), GeneralFrame::GeneralFrameName);
  }

  // Have the factory create it
  IMDEventWorkspace_sptr out = MDEventFactory::CreateMDWorkspace(ndims, eventType);

  // Give all the dimensions
  for (size_t d = 0; d < ndims; d++) {
    auto frame = createMDFrame(frames[d], units[d]);
    MDHistoDimension *dim = new MDHistoDimension(names[d], names[d], *frame, static_cast<coord_t>(extents[d * 2]),
                                                 static_cast<coord_t>(extents[d * 2 + 1]), 1);
    out->addDimension(MDHistoDimension_sptr(dim));
  }

  // Initialize it using the dimension
  out->initialize();

  // Call the templated function to finish ints
  CALL_MDEVENT_FUNCTION(this->finish, out);

  // --- File back end ? ----------------
  std::string filename = getProperty("Filename");
  if (!filename.empty()) {
    // First save to the NXS file
    g_log.notice() << "Running SaveMD\n";
    auto alg = createChildAlgorithm("SaveMD");
    alg->setPropertyValue("Filename", filename);
    alg->setProperty("InputWorkspace", std::dynamic_pointer_cast<IMDWorkspace>(out));
    alg->executeAsChildAlg();
    // And now re-load it with this file as the backing.
    g_log.notice() << "Running LoadMD\n";
    alg = createChildAlgorithm("LoadMD");
    alg->setPropertyValue("Filename", filename);
    alg->setProperty("FileBackEnd", true);
    alg->setPropertyValue("Memory", getPropertyValue("Memory"));
    alg->executeAsChildAlg();
    // Replace the workspace with the loaded, file-backed one
    IMDWorkspace_sptr temp;
    temp = alg->getProperty("OutputWorkspace");
    out = std::dynamic_pointer_cast<IMDEventWorkspace>(temp);
  }

  // Save it on the output.
  setProperty("OutputWorkspace", std::dynamic_pointer_cast<Workspace>(out));
}

MDFrame_uptr CreateMDWorkspace::createMDFrame(const std::string &frame, const std::string &unit) {
  auto frameFactory = makeMDFrameFactoryChain();
  MDFrameArgument frameArg(frame, unit);
  return frameFactory->create(frameArg);
}

std::map<std::string, std::string> CreateMDWorkspace::validateInputs() {
  // Check Frame names
  std::map<std::string, std::string> errors;
  std::string framePropertyName = "Frames";
  std::vector<std::string> frames = getProperty(framePropertyName);
  int ndims_prop = getProperty("Dimensions");
  auto ndims = static_cast<size_t>(ndims_prop);

  std::vector<std::string> targetFrames{Mantid::Geometry::GeneralFrame::GeneralFrameName,
                                        Mantid::Geometry::HKL::HKLName, Mantid::Geometry::QLab::QLabName,
                                        Mantid::Geometry::QSample::QSampleName};

  auto isValidFrame = true;
  for (auto &frame : frames) {
    auto result = checkIfFrameValid(frame, targetFrames);
    if (!result) {
      isValidFrame = result;
    }
  }

  if (!frames.empty() && frames.size() != ndims) {
    isValidFrame = false;
  }

  if (!isValidFrame) {
    std::string message = "The selected frames can be 'HKL', 'QSample', 'QLab' "
                          "or 'General Frame'. You must specify as many frames "
                          "as there are dimensions.";
    errors.emplace(framePropertyName, message);
  }
  return errors;
}

/**
 * Check if the specified frame matches a target frame
 * @param frame: the frame name under investigation
 * @param targetFrames: the allowed frame names
 * @returns true if the frame name is valid else false
 */
bool CreateMDWorkspace::checkIfFrameValid(const std::string &frame, const std::vector<std::string> &targetFrames) {
  return std::any_of(targetFrames.cbegin(), targetFrames.cend(),
                     [&frame](const auto &targetFrame) { return targetFrame == frame; });
}

} // namespace Mantid::MDAlgorithms
