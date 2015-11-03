#include "MantidMDAlgorithms/SetMDFrames.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/ListValidator.h"

#include <map>
#include <boost/pointer_cast.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetMDFrames)

const std::string SetMDFrames::mdFrameSpecifier = "MDFrame";

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SetMDFrames::SetMDFrames() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SetMDFrames::~SetMDFrames() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SetMDFrames::name() const { return "SetMDFrames"; }

/// Algorithm's version for identification. @see Algorithm::version
int SetMDFrames::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetMDFrames::category() const {
  return "MDAlgorithms";
  ;
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SetMDFrames::summary() const {
  return "Set the MDFrame for each axis for legacy MDHisto and MDEvent "
         "workspaces.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SetMDFrames::init() {
  declareProperty(new WorkspaceProperty<Mantid::API::IMDWorkspace>(
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
  std::string dimChars = getDimensionChars();
  for (size_t i = 0; i < dimChars.size(); i++) {
    std::string dim(" ");
    dim[0] = dimChars[i];
    std::string propName = mdFrameSpecifier + dim;

    declareProperty(
        propName, Mantid::Geometry::GeneralFrame::GeneralFrameName,
        boost::make_shared<Mantid::Kernel::StringListValidator>(mdFrames),
        "MDFrame selection for the " + Mantid::Kernel::Strings::toString(i) +
            "th dimension.\n");

    setPropertyGroup(propName, "MDFrames");
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SetMDFrames::exec() {
  Mantid::API::IMDWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  // Get the units information for each dimension
  auto numberOfDimensions = inputWorkspace->getNumDims();
  for (size_t index = 0; index < numberOfDimensions; ++index) {
    auto dimension = inputWorkspace->getDimension(index);

    // Get frame specifier
    std::string propertyID =
        mdFrameSpecifier + boost::lexical_cast<std::string>(index);
    std::string frameSelection = getProperty(propertyID);

    // Provide a new MDFrame
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
      throw std::runtime_error(
          "SetMDFrames: Cannot convert to MDHistDimension");
    }
    mdHistoDimension->setMDFrame(*newMDFrame);
  }
}

/**
 * Check the inputs for invalid values
 * @returns A map with validation warnings.
 */
std::map<std::string, std::string> SetMDFrames::validateInputs() {
  std::map<std::string, std::string> invalidProperties;
  Mantid::API::IMDWorkspace_sptr ws = getProperty("InputWorkspace");

  if (!boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws) &&
      !boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(ws)) {
    invalidProperties.insert(
        std::make_pair("InputWorkspace", "The input workspace has to be either "
                                         "an MDEvent or MDHisto Workspace."));
  }
  return invalidProperties;
}

/**
 * Creates an MDFrame based on the users selection
 * @param
 */
Mantid::Geometry::MDFrame_uptr
SetMDFrames::createMDFrame(const std::string &frameSelection,
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
      throw std::invalid_argument("SetMDFrames: " + frameSelection +
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
        "SetMDFrames: The selected MDFrame does not seem to be supported");
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
