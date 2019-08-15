// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNexusGeometry.h"

namespace Mantid {
namespace DataHandling {
using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNexusGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SaveNexusGeometry::name() const { return "SaveNexusGeometry"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveNexusGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveNexusGeometry::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveNexusGeometry::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveNexusGeometry::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "",
                                                             Direction::Input),
      "An input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "",
                                                             Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveNexusGeometry::exec() {
  // TODO Auto-generated execute stub
}

} // namespace DataHandling
} // namespace Mantid
