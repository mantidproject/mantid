// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SetSpecialCoordinates.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("SetSpecialCoordinates");
}

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetSpecialCoordinates)

const std::string SetSpecialCoordinates::QLabOption() {
  static const std::string ret("Q (lab frame)");
  return ret;
}

const std::string SetSpecialCoordinates::QSampleOption() {
  static const std::string ret("Q (sample frame)");
  return ret;
}

const std::string SetSpecialCoordinates::HKLOption() {
  static const std::string ret("HKL");
  return ret;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SetSpecialCoordinates::SetSpecialCoordinates() {
  m_specialCoordinatesNames.push_back(SetSpecialCoordinates::QLabOption());
  m_specialCoordinatesNames.push_back(SetSpecialCoordinates::QSampleOption());
  m_specialCoordinatesNames.push_back(SetSpecialCoordinates::HKLOption());

  m_specialCoordinatesMap.emplace(SetSpecialCoordinates::QLabOption(),
                                  Mantid::Kernel::QLab);
  m_specialCoordinatesMap.emplace(SetSpecialCoordinates::QSampleOption(),
                                  Mantid::Kernel::QSample);
  m_specialCoordinatesMap.emplace(SetSpecialCoordinates::HKLOption(),
                                  Mantid::Kernel::HKL);
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SetSpecialCoordinates::name() const {
  return "SetSpecialCoordinates";
}

/// Algorithm's version for identification. @see Algorithm::version
int SetSpecialCoordinates::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetSpecialCoordinates::category() const {
  return "Crystal\\Corrections";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SetSpecialCoordinates::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "",
                                                     Direction::InOut),
      "An input/output workspace. The new log will be added to it. Important "
      "Note: This has now only an effect on PeaksWorkspaces. MDEvent and "
      "MDHisto worksapces are not affaceted by this algorithm");

  declareProperty(
      "SpecialCoordinates", "Q (lab frame)",
      boost::make_shared<StringListValidator>(m_specialCoordinatesNames),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of "
      "the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices.");
}

bool SetSpecialCoordinates::writeCoordinatesToMDEventWorkspace(
    Workspace_sptr inWS, SpecialCoordinateSystem /*unused*/) {
  bool written = false;
  if (auto mdEventWS = boost::dynamic_pointer_cast<IMDEventWorkspace>(inWS)) {
    g_log.warning("SetSpecialCoordinates: This algorithm cannot set the "
                  "special coordinate system for an MDEvent workspace any "
                  "longer.");
    written = true;
  }
  return written;
}

bool SetSpecialCoordinates::writeCoordinatesToMDHistoWorkspace(
    Workspace_sptr inWS, SpecialCoordinateSystem /*unused*/) {
  bool written = false;
  if (auto mdHistoWS = boost::dynamic_pointer_cast<IMDHistoWorkspace>(inWS)) {
    g_log.warning("SetSpecialCoordinates: This algorithm cannot set the "
                  "special coordinate system for an MDHisto workspace any "
                  "longer.");
    written = true;
  }
  return written;
}

bool SetSpecialCoordinates::writeCoordinatesToPeaksWorkspace(
    Workspace_sptr inWS, SpecialCoordinateSystem coordinateSystem) {
  bool written = false;
  if (auto peaksWS = boost::dynamic_pointer_cast<IPeaksWorkspace>(inWS)) {
    peaksWS->setCoordinateSystem(coordinateSystem);
    written = true;
  }
  return written;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SetSpecialCoordinates::exec() {

  Workspace_sptr inputWS = getProperty("InputWorkspace");

  std::string requestedCoordinateSystem = getProperty("SpecialCoordinates");

  SpecialCoordinateSystem coordinatesToUse =
      this->m_specialCoordinatesMap.find(requestedCoordinateSystem)->second;

  // Try to write the coordinates to the various allowed types of workspace.
  if (!writeCoordinatesToMDEventWorkspace(inputWS, coordinatesToUse)) {
    if (!writeCoordinatesToMDHistoWorkspace(inputWS, coordinatesToUse)) {
      if (!writeCoordinatesToPeaksWorkspace(inputWS, coordinatesToUse)) {
        throw std::invalid_argument(
            "A workspace of this type cannot be processed/");
      }
    }
  }
}

} // namespace Crystal
} // namespace Mantid
