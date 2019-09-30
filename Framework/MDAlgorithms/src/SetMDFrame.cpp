// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/SetMDFrame.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MDAxisValidator.h"

#include <boost/make_shared.hpp>
#include <boost/pointer_cast.hpp>
#include <map>

namespace Mantid {
namespace MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetMDFrame)

const std::string SetMDFrame::mdFrameSpecifier = "MDFrame";

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SetMDFrame::name() const { return "SetMDFrame"; }

/// Algorithm's version for identification. @see Algorithm::version
int SetMDFrame::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetMDFrame::category() const {
  return "MDAlgorithms";
  ;
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SetMDFrame::summary() const {
  return "Sets a new MDFrame type for a selection of axes for legacy MDHisto "
         "and MDEvent "
         "workspaces.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SetMDFrame::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Mantid::API::IMDWorkspace>>(
          "InputWorkspace", "", Direction::InOut),
      "The workspace for which the MDFrames are to be changed. "
      "Note that only MDHisto and MDEvent workspaces can be "
      "altered by this algorithm.");

  // Options for the MDFrames
  std::vector<std::string> mdFrames;
  mdFrames.push_back(Mantid::Geometry::GeneralFrame::GeneralFrameName);
  mdFrames.push_back(Mantid::Geometry::QSample::QSampleName);
  mdFrames.push_back(Mantid::Geometry::QLab::QLabName);
  mdFrames.push_back(Mantid::Geometry::HKL::HKLName);
  mdFrames.push_back(Mantid::Geometry::UnknownFrame::UnknownFrameName);

  // Create a selection of MDFrames and units for each dimension
  std::string propName = mdFrameSpecifier;

  declareProperty(
      propName, Mantid::Geometry::GeneralFrame::GeneralFrameName,
      boost::make_shared<Mantid::Kernel::StringListValidator>(mdFrames),
      "MDFrame type selection.\n");

  auto axisValidator =
      boost::make_shared<Mantid::Kernel::ArrayBoundedValidator<int>>();
  axisValidator->setLower(0);
  declareProperty(
      std::make_unique<Kernel::ArrayProperty<int>>(
          "Axes", std::vector<int>(0), axisValidator, Direction::Input),
      "Selects the axes which are going to be set to the new MDFrame type.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SetMDFrame::exec() {
  Mantid::API::IMDWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  std::vector<int> axesInts = this->getProperty("Axes");
  std::vector<size_t> axes(axesInts.begin(), axesInts.end());

  // If no axes were specified, then don't do anything
  if (axes.empty()) {
    return;
  }

  for (auto &axe : axes) {
    // Get associated dimension
    auto dimension = inputWorkspace->getDimension(axe);

    // Provide a new MDFrame
    std::string frameSelection = getProperty(mdFrameSpecifier);
    const auto &mdFrame = dimension->getMDFrame();
    auto newMDFrame = createMDFrame(frameSelection, mdFrame);

    // Set the new MDFrame. We need to alter the MDFrame information
    // of the MDHistoDimension which we only get as a const -- hence
    // we need a const-cast at this point.
    auto mdHistoDimension =
        boost::const_pointer_cast<Mantid::Geometry::MDHistoDimension>(
            boost::dynamic_pointer_cast<
                const Mantid::Geometry::MDHistoDimension>(dimension));
    if (!mdHistoDimension) {
      throw std::runtime_error("SetMDFrame: Cannot convert to MDHistDimension");
    }
    mdHistoDimension->setMDFrame(*newMDFrame);
  }
}

/**
 * Check the inputs for invalid values
 * @returns A map with validation warnings.
 */
std::map<std::string, std::string> SetMDFrame::validateInputs() {
  std::map<std::string, std::string> invalidProperties;
  Mantid::API::IMDWorkspace_sptr ws = getProperty("InputWorkspace");

  if (!boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws) &&
      !boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws)) {
    invalidProperties.insert(
        std::make_pair("InputWorkspace", "The input workspace has to be either "
                                         "an MDEvent or MDHisto Workspace."));
  }

  std::vector<int> axesInts = this->getProperty("Axes");
  Kernel::MDAxisValidator axisChecker(axesInts, ws->getNumDims(), true);
  auto axisErrors = axisChecker.validate();
  for (auto &axisError : axisErrors) {
    invalidProperties.insert(axisError);
  }

  return invalidProperties;
}

/**
 * Creates an MDFrame based on the users selection
 * @param frameSelection :: the selected frame type
 * @param oldFrame :: a refrence to the frame we want to replace
 * @returns :: a unique pointer to the new MDFrame
 */
Mantid::Geometry::MDFrame_uptr
SetMDFrame::createMDFrame(const std::string &frameSelection,
                          const Mantid::Geometry::MDFrame &oldFrame) const {
  auto mdFrameFactory = Mantid::Geometry::makeMDFrameFactoryChain();
  if (frameSelection == Mantid::Geometry::GeneralFrame::GeneralFrameName) {
    Mantid::Geometry::MDFrameArgument argument(
        Mantid::Geometry::GeneralFrame::GeneralFrameName,
        oldFrame.getUnitLabel());
    return mdFrameFactory->create(argument);
  } else if (frameSelection == Mantid::Geometry::QSample::QSampleName) {
    Mantid::Geometry::MDFrameArgument argument(
        Mantid::Geometry::QSample::QSampleName);
    return mdFrameFactory->create(argument);
  } else if (frameSelection == Mantid::Geometry::QLab::QLabName) {
    Mantid::Geometry::MDFrameArgument argument(
        Mantid::Geometry::QLab::QLabName);
    return mdFrameFactory->create(argument);
  } else if (frameSelection == Mantid::Geometry::HKL::HKLName) {
    Mantid::Geometry::MDFrameArgument argument(Mantid::Geometry::HKL::HKLName,
                                               oldFrame.getUnitLabel());
    // We want to make sure here that we get an HKL MDFrame, hence we need to
    // make sure that the HKL frame accepts the units
    Mantid::Geometry::HKLFrameFactory hklFrameFactory;
    auto canInterpret = hklFrameFactory.canInterpret(argument);
    if (!canInterpret) {
      throw std::invalid_argument("SetMDFrame: " + frameSelection +
                                  " does not have units which are compatible "
                                  "with an HKL frame. Please contact the "
                                  "Mantid team if you believe that the units "
                                  "should be compatible.");
    }

    return mdFrameFactory->create(argument);
  } else if (frameSelection ==
             Mantid::Geometry::UnknownFrame::UnknownFrameName) {
    Mantid::Geometry::MDFrameArgument argument(
        Mantid::Geometry::UnknownFrame::UnknownFrameName,
        oldFrame.getUnitLabel());
    return mdFrameFactory->create(argument);
  } else {
    throw std::invalid_argument(
        "SetMDFrame: The selected MDFrame does not seem to be supported");
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
