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
  return "Algorithm takes two table workspaces and, if they have the same column titles and data types, combines them "
         "into a single table. Currently supports data types of double, int, float, string, bool, size_t and V3D.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CombineTableWorkspaces::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("LHSWorkspace", "", Direction::Input),
      "The first table workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("RHSWorkspace", "", Direction::Input),
      "The second table workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::TableWorkspace>>("OutputWorkspace", "", Direction::Output),
      "The combined table workspace.");
}

const std::map<std::string, int> &CombineTableWorkspaces::allowedTypes() {
  static const std::map<std::string, int> types = {{"double", 0}, {"int", 1},   {"str", 2}, {"bool", 3},
                                                   {"size_t", 4}, {"float", 5}, {"V3D", 6}};
  return types;
}

std::map<std::string, std::string> CombineTableWorkspaces::validateInputs() {
  std::map<std::string, std::string> results;

  const DataObjects::TableWorkspace_sptr LHSWorkspace = getProperty("LHSWorkspace");
  const DataObjects::TableWorkspace_sptr RHSWorkspace = getProperty("RHSWorkspace");

  const auto expectedCols = LHSWorkspace->columnCount();

  // check correct number of columns
  if (RHSWorkspace->columnCount() != expectedCols) {
    results.emplace("LHSWorkspace", "Both Table Workspaces must have the same number of columns");
    results.emplace("RHSWorkspace", "Both Table Workspaces must have the same number of columns");
    return results;
  }

  // get column titles
  const auto lColNames = LHSWorkspace->getColumnNames();
  const auto rColNames = RHSWorkspace->getColumnNames();

  const std::map<std::string, int> &allowedColumnTypes = allowedTypes();

  bool matchingColumnNames = true;
  bool matchingColumnTypes = true;
  bool allColumnTypesAllowed = true;

  for (int i = 0; i < static_cast<int>(expectedCols); i++) {
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
  }

  if (!matchingColumnTypes) {
    results.emplace("LHSWorkspace", "Both Table Workspaces must have the same data types for corresponding columns");
    results.emplace("RHSWorkspace", "Both Table Workspaces must have the same data types for corresponding columns");
  }

  if (!allColumnTypesAllowed) {
    results.emplace("LHSWorkspace", "Only supported data types are: double, int, string, bool, size_t, float, and V3D");
    results.emplace("RHSWorkspace", "Only supported data types are: double, int, string, bool, size_t, float, and V3D");
  }

  return results;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CombineTableWorkspaces::exec() {
  const DataObjects::TableWorkspace_sptr LHSWorkspace = getProperty("LHSWorkspace");
  const DataObjects::TableWorkspace_sptr RHSWorkspace = getProperty("RHSWorkspace");

  const std::map<std::string, int> &allowedColumnTypes = allowedTypes();

  // Copy the first workspace to our output workspace
  DataObjects::TableWorkspace_sptr outputWS = LHSWorkspace->clone();
  // Get hold of the peaks in the second workspace
  const int nRows = static_cast<int>(RHSWorkspace->rowCount());
  const int nCols = static_cast<int>(RHSWorkspace->columnCount());

  std::vector<std::string> colTypes = {};
  for (auto i = 0; i < nCols; i++) {
    colTypes.emplace_back(RHSWorkspace->getColumn(i)->type());
  }

  Progress progress(this, 0.0, 1.0, nRows);

  for (int r = 0; r < nRows; r++) {
    TableRow newRow = outputWS->appendRow();
    TableRow currentRow = RHSWorkspace->getRow(r);
    for (int c = 0; c < nCols; c++) {
      const auto dType = colTypes[c];
      const int dTypeVal = allowedColumnTypes.at(dType);
      switch (dTypeVal) {
      case 0:
        newRow << currentRow.Double(c);
        break;
      case 1:
        newRow << currentRow.Int(c);
        break;
      case 2:
        newRow << currentRow.cell<std::string>(c);
        break;
      case 3:
        newRow << currentRow.cell<Mantid::API::Boolean>(c);
        break;
      case 4:
        newRow << currentRow.cell<std::size_t>(c);
        break;
      case 5:
        newRow << currentRow.cell<float>(c);
        break;
      case 6:
        newRow << currentRow.cell<V3D>(c);
        break;
      default:
        // this should be caught by the input validation anyway
        throw std::runtime_error("Unsupported Data Type");
      }
    }
    progress.report();
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
