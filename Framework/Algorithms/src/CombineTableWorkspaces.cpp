// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CombineTableWorkspaces.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <any>

namespace Mantid::Algorithms {
using Mantid::API::Progress;
using Mantid::API::TableRow;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

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

std::map<std::string, std::string> CombineTableWorkspaces::validateInputs() {
  std::map<std::string, std::string> results;

  const DataObjects::TableWorkspace_sptr LHSWorkspace = getProperty("LHSWorkspace");
  const DataObjects::TableWorkspace_sptr RHSWorkspace = getProperty("RHSWorkspace");
  DataObjects::TableWorkspace_sptr outputWS = getProperty("OutputWorkspace");

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

  bool matchingColumnNames = true;
  bool matchingColumnTypes = true;

  for (auto i = 0; i < expectedCols; i++) {
    if (lColNames[i] != rColNames[i]) {
      matchingColumnNames = false;
      break;
    }
    if (LHSWorkspace->getColumn(i)->type() != RHSWorkspace->getColumn(i)->type()) {
      matchingColumnTypes = false;
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

  return results;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CombineTableWorkspaces::exec() {
  const DataObjects::TableWorkspace_sptr LHSWorkspace = getProperty("LHSWorkspace");
  const DataObjects::TableWorkspace_sptr RHSWorkspace = getProperty("RHSWorkspace");

  const std::string doubleType = "double";
  const std::string intType = "int";
  const std::string stringType = "str";
  const std::string boolType = "bool";

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
      if (colTypes[c] == doubleType) {
        newRow << currentRow.Double(c);
      } else if (colTypes[c] == intType) {
        newRow << currentRow.Int(c);
      } else if (colTypes[c] == stringType) {
        std::string val = currentRow.cell<std::string>(c);
        newRow << val;
      } else if (colTypes[c] == boolType) {
        bool val = currentRow.cell<Mantid::API::Boolean>(c);
        newRow << val;
      }
    }
    progress.report();
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
