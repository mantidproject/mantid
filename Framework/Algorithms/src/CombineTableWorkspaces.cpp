// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CombineTableWorkspaces.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid::Algorithms {
using Mantid::API::Progress;
using Mantid::API::TableRow;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::V3D;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CombineTableWorkspaces)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CombineTableWorkspaces::name() const { return "CombineTableWorkspaces"; }

/// Algorithm's version for identification. @see Algorithm::version
int CombineTableWorkspaces::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CombineTableWorkspaces::category() const { return "Utility\\Workspaces"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CombineTableWorkspaces::summary() const {
  return "Combine a pair of table workspaces into a single table workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CombineTableWorkspaces::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("LHSWorkspace", "", Direction::Input),
      "The first set of peaks.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("RHSWorkspace", "", Direction::Input),
      "The second set of peaks.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("OutputWorkspace", "", Direction::Output),
      "The combined peaks list.");
}

const std::map<std::string, int> CombineTableWorkspaces::allowedTypes() {
  return {{"double", 0}, {"int", 1}, {"str", 2}, {"bool", 3}, {"size_t", 4}, {"float", 5}, {"V3D", 6}};
}

std::map<std::string, std::string> CombineTableWorkspaces::validateInputs() {
  std::map<std::string, std::string> results;

  const DataObjects::TableWorkspace_sptr LHSWorkspace = getProperty("LHSWorkspace");
  const DataObjects::TableWorkspace_sptr RHSWorkspace = getProperty("RHSWorkspace");

  auto expectedCols = LHSWorkspace->columnCount();

  // check correct number of columns
  if (RHSWorkspace->columnCount() != expectedCols) {
    results.emplace("LHSWorkspace", "Both Table Workspaces must have the same number of columns");
    results.emplace("RHSWorkspace", "Both Table Workspaces must have the same number of columns");
    return results;
  }

  // get column titles
  auto lColNames = LHSWorkspace->getColumnNames();
  auto rColNames = RHSWorkspace->getColumnNames();

  const std::map<std::string, int> allowedColumnTypes = allowedTypes();

  bool matchingColumnNames = true;
  bool matchingColumnTypes = true;
  bool allColumnTypesAllowed = true;

  for (auto i = 0; i < expectedCols; i++) {
    if (lColNames[i] != rColNames[i]) {
      matchingColumnNames = false;
      break;
    }
    auto LHType = LHSWorkspace->getColumn(i)->type();
    if (LHType != RHSWorkspace->getColumn(i)->type()) {
      matchingColumnTypes = false;
      break;
    }
    auto it = allowedColumnTypes.find(LHType);
    if (it == allowedColumnTypes.end()) {
      allColumnTypesAllowed = false;
      break;
    }
  }

  if (!matchingColumnNames) {
    results.emplace("LHSWorkspace", "Both Table Workspaces must have the same column titles");
    results.emplace("RHSWorkspace", "Both Table Workspaces must have the same column titles");
    return results;
  }

  if (!matchingColumnTypes) {
    results.emplace("LHSWorkspace", "Both Table Workspaces must have the same data types for corresponding columns");
    results.emplace("RHSWorkspace", "Both Table Workspaces must have the same data types for corresponding columns");
    return results;
  }

  if (!allColumnTypesAllowed) {
    results.emplace("LHSWorkspace", "Only supported data types are: double, int, string, bool, size_t, float, and V3D");
    results.emplace("RHSWorkspace", "Only supported data types are: double, int, string, bool, size_t, float, and V3D");
    return results;
  }

  return results;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CombineTableWorkspaces::exec() {
  const DataObjects::TableWorkspace_sptr LHSWorkspace = getProperty("LHSWorkspace");
  const DataObjects::TableWorkspace_sptr RHSWorkspace = getProperty("RHSWorkspace");

  const std::map<std::string, int> allowedColumnTypes = allowedTypes();

  // Copy the first workspace to our output workspace
  DataObjects::TableWorkspace_sptr outputWS = LHSWorkspace->clone();
  // Get hold of the peaks in the second workspace
  auto nRows = RHSWorkspace->rowCount();
  auto nCols = RHSWorkspace->columnCount();

  std::vector<std::string> colTypes = {};
  for (auto i = 0; i < RHSWorkspace->columnCount(); i++) {
    colTypes.emplace_back(RHSWorkspace->getColumn(i)->type());
  }

  Progress progress(this, 0.0, 1.0, nRows);

  for (std::size_t r = 0; r < nRows; r++) {
    TableRow newRow = outputWS->appendRow();
    TableRow currentRow = RHSWorkspace->getRow(r);
    for (std::size_t c = 0; c < nCols; c++) {
      auto dType = colTypes[c];
      if (allowedColumnTypes.at(dType) == 0) {
        newRow << currentRow.Double(c);
      } else if (allowedColumnTypes.at(dType) == 1) {
        newRow << currentRow.Int(c);
      } else if (allowedColumnTypes.at(dType) == 2) {
        std::string val = currentRow.cell<std::string>(c);
        newRow << val;
      } else if (allowedColumnTypes.at(dType) == 3) {
        bool val = currentRow.cell<Mantid::API::Boolean>(c);
        newRow << val;
      } else if (allowedColumnTypes.at(dType) == 4) {
        std::size_t val = currentRow.cell<std::size_t>(c);
        newRow << val;
      } else if (allowedColumnTypes.at(dType) == 5) {
        float val = currentRow.cell<float>(c);
        newRow << val;
      } else if (allowedColumnTypes.at(dType) == 6) {
        V3D val = currentRow.cell<V3D>(c);
        newRow << val;
      }
    }
    progress.report();
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
