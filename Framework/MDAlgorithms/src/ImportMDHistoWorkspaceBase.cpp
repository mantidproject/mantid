// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ImportMDHistoWorkspaceBase.h"

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace MDAlgorithms {

/**
Functor to compute the product of the set.
*/
struct Product : public std::unary_function<size_t, void> {
  Product() : result(1) {}
  size_t result;
  void operator()(size_t x) { result *= x; }
};

//----------------------------------------------------------------------------------------------
/** Initalise generic importing properties.
 */
void ImportMDHistoWorkspaceBase::initGenericImportProps() {
  auto validator = boost::make_shared<CompositeValidator>();
  validator->add(boost::make_shared<BoundedValidator<int>>(1, 9));
  validator->add(boost::make_shared<MandatoryValidator<int>>());

  declareProperty(make_unique<PropertyWithValue<int>>(
                      "Dimensionality", -1, validator, Direction::Input),
                  "Dimensionality of the data in the file.");

  declareProperty(make_unique<ArrayProperty<double>>("Extents"),
                  "A comma separated list of min, max for each dimension,\n"
                  "specifying the extents of each dimension.");

  declareProperty(make_unique<ArrayProperty<int>>("NumberOfBins"),
                  "Number of bin in each dimension.");

  declareProperty(make_unique<ArrayProperty<std::string>>("Names"),
                  "A comma separated list of the name of each dimension.");

  declareProperty(make_unique<ArrayProperty<std::string>>("Units"),
                  "A comma separated list of the units of each dimension.");

  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "MDHistoWorkspace reflecting the input text file.");
  declareProperty(
      make_unique<ArrayProperty<std::string>>("Frames"),
      " A comma separated list of the frames of each dimension. "
      " The frames can be"
      " **General Frame**: Any frame which is not a Q-based frame."
      " **QLab**: Wave-vector converted into the lab frame."
      " **QSample**: Wave-vector converted into the frame of the sample."
      " **HKL**: Wave-vector converted into the crystal's HKL indices."
      " Note if nothing is specified then the **General Frame** is being "
      "selected. Also note that if you select a frame then this might override "
      "your unit selection if it is not compatible with the frame.");
}

//----------------------------------------------------------------------------------------------
/** Create an empty output workspace from the generic properies. This gives a
new workspace with dimensions provided, but signal and
error arrays will not yet be set.
*/
MDHistoWorkspace_sptr ImportMDHistoWorkspaceBase::createEmptyOutputWorkspace() {
  // Fetch input properties
  size_t ndims;
  {
    int ndims_int = getProperty("Dimensionality");
    ndims = ndims_int;
  }
  std::vector<double> extents = getProperty("Extents");
  std::vector<int> nbins = getProperty("NumberOfBins");
  std::vector<std::string> names = getProperty("Names");
  std::vector<std::string> units = getProperty("Units");
  std::vector<std::string> frames = getProperty("Frames");

  // Perform all validation on inputs
  if (extents.size() != ndims * 2)
    throw std::invalid_argument("You must specify twice as many extents "
                                "(min,max) as there are dimensions.");
  if (nbins.size() != ndims)
    throw std::invalid_argument(
        "You must specify as number of bins as there are dimensions.");
  if (names.size() != ndims)
    throw std::invalid_argument(
        "You must specify as many names as there are dimensions.");
  if (units.size() != ndims)
    throw std::invalid_argument(
        "You must specify as many units as there are dimensions.");

  // If no frames are specified we want to default to the General Frame,
  // to ensure backward compatibility. But if they are only partly specified,
  // then we want to throw an error. It should be either used correctly or not
  // at all
  if (!frames.empty() && frames.size() != ndims) {
    throw std::invalid_argument(
        "You must specify as many frames as there are dimensions.");
  }

  if (frames.empty()) {
    frames.resize(ndims);
    std::fill(frames.begin(), frames.end(), GeneralFrame::GeneralFrameName);
  }

  // Fabricate new dimensions from inputs
  std::vector<MDHistoDimension_sptr> dimensions;
  for (size_t k = 0; k < ndims; ++k) {
    auto frame = createMDFrame(frames[k], units[k]);
    dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(
        names[k], names[k], *frame, static_cast<coord_t>(extents[k * 2]),
        static_cast<coord_t>(extents[(k * 2) + 1]), nbins[k])));
  }

  // Calculate the total number of bins by multiplying across each dimension.
  Product answer = std::for_each(nbins.begin(), nbins.end(), Product());
  m_bin_product = answer.result;

  MDHistoWorkspace_sptr ws(new MDHistoWorkspace(dimensions));
  return ws;
}

/**
 * Create an MDFrame
 * @param frame: the selected frame
 * @param unit: the selected unit
 * @returns a unique pointer to an MDFrame
 */
MDFrame_uptr ImportMDHistoWorkspaceBase::createMDFrame(std::string frame,
                                                       std::string unit) {
  auto frameFactory = makeMDFrameFactoryChain();
  MDFrameArgument frameArg(frame, unit);
  return frameFactory->create(frameArg);
}

std::map<std::string, std::string>
ImportMDHistoWorkspaceBase::validateInputs() {
  // Check Frame names
  std::map<std::string, std::string> errors;
  std::string framePropertyName = "Frames";
  std::vector<std::string> frames = getProperty(framePropertyName);
  int ndims_prop = getProperty("Dimensionality");
  auto ndims = static_cast<size_t>(ndims_prop);

  std::vector<std::string> targetFrames;
  targetFrames.push_back(Mantid::Geometry::GeneralFrame::GeneralFrameName);
  targetFrames.push_back(Mantid::Geometry::HKL::HKLName);
  targetFrames.push_back(Mantid::Geometry::QLab::QLabName);
  targetFrames.push_back(Mantid::Geometry::QSample::QSampleName);

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
bool ImportMDHistoWorkspaceBase::checkIfFrameValid(
    const std::string &frame, const std::vector<std::string> &targetFrames) {
  return std::any_of(
      targetFrames.cbegin(), targetFrames.cend(),
      [&frame](const auto &targetFrame) { return targetFrame == frame; });
}

} // namespace MDAlgorithms
} // namespace Mantid
