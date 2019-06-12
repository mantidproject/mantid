// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/WorkflowAlgorithmRunner.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"

#include <deque>
#include <unordered_map>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

namespace PropertyNames {
const static std::string ALGORITHM("Algorithm");
const static std::string IO_MAP("InputOutputMap");
const static std::string SETUP_TABLE("SetupTable");
} // namespace PropertyNames

/** Checks if a string is a hard coded workspace name.
 * @param s string to be checked
 * @return return true if `s` is surrounded by '"' or '''
 */
bool isHardCodedWorkspaceName(const std::string &s) {
  if (s.size() < 3) {
    return false;
  }
  const auto f = s.front();
  const auto b = s.back();
  return (f == '\'' && b == '\'') || (f == '"' && b == '"');
}

/** Removes first and last character of a string.
 * @param s string to be tidied
 * @return tidied string
 * @throw std::runtime_error if `s` is too short
 */
std::string tidyWorkspaceName(const std::string &s) {
  if (s.length() < 3) {
    throw std::runtime_error("Workspace name is too short.");
  }
  return std::string(s.cbegin() + 1, s.cend() - 1);
}

/** Transforms a setup table to an empty property table.
 * Clear cells referred to in `ioMapping`, if they do not contain
 * hard coded values. In the case of hard coded values, removes
 * '"' and ''' characters.
 * @tparam MAP a type with `std::map` like iterators
 * @param[in,out] table the table to be cleaned
 * @param[in] ioMapping input/output property lookup table
 * @throw std::runtime_error in case of errorneous entries in `table`
 */
template <typename MAP>
void cleanPropertyTable(ITableWorkspace_sptr table, const MAP &ioMapping) {
  // Some output columns may be processed several times, but this should
  // not be a serious performance hit.
  for (const auto &ioPair : ioMapping) {
    Column_sptr inputColumn = table->getColumn(ioPair.first);
    Column_sptr outputColumn = table->getColumn(ioPair.second);
    for (size_t i = 0; i < table->rowCount(); ++i) {
      inputColumn->cell<std::string>(i).clear();
      std::string &outputValue = outputColumn->cell<std::string>(i);
      if (!isHardCodedWorkspaceName(outputValue)) {
        outputValue.clear();
      }
    }
  }
  // Second pass: tidy hard-coded outputs.
  // This could not be done in first pass as multiple inputs may depend
  // on single output and then the hard-coded values would be cleared.
  for (const auto &ioPair : ioMapping) {
    Column_sptr outputColumn = table->getColumn(ioPair.second);
    for (size_t i = 0; i < table->rowCount(); ++i) {
      std::string &outputValue = outputColumn->cell<std::string>(i);
      if (isHardCodedWorkspaceName(outputValue)) {
        outputValue = tidyWorkspaceName(outputValue);
      }
    }
  }
}

// Register the algorithm into the algorithm factory.
DECLARE_ALGORITHM(WorkflowAlgorithmRunner)

void WorkflowAlgorithmRunner::init() {
  declareProperty(PropertyNames::ALGORITHM, "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Name of the algorithm to run");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      PropertyNames::SETUP_TABLE.c_str(), "", Direction::Input),
                  "Table workspace containing the setup of the runs.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      PropertyNames::IO_MAP.c_str(), "", Direction::Input),
                  "Table workspace mapping algorithm outputs to inputs.");
}

void WorkflowAlgorithmRunner::exec() {
  ITableWorkspace_sptr setupTable = getProperty(PropertyNames::SETUP_TABLE);
  // propertyTable will contain the final properties of the managed algorithms.
  ITableWorkspace_sptr propertyTable = setupTable->clone();
  ITableWorkspace_sptr ioMappingTable = getProperty(PropertyNames::IO_MAP);
  if (ioMappingTable->rowCount() != 1) {
    throw std::runtime_error("Row count in " + PropertyNames::IO_MAP + " is " +
                             std::to_string(ioMappingTable->rowCount()) +
                             ", not 1.");
  }
  // Transform ioMappingTable to a real lookup table (ioMap) and check for
  // consistency.
  std::unordered_map<std::string, std::string> ioMap;
  const auto inputPropertyNames = ioMappingTable->getColumnNames();
  for (size_t col = 0; col < ioMappingTable->columnCount(); ++col) {
    const auto &inputPropertyName = ioMappingTable->getColumn(col)->name();
    const auto &outputPropertyName = ioMappingTable->String(0, col);
    if (!outputPropertyName.empty()) {
      if (std::find(inputPropertyNames.cbegin(), inputPropertyNames.cend(),
                    outputPropertyName) != inputPropertyNames.cend()) {
        throw std::runtime_error("Property " + outputPropertyName +
                                 " linked to " + inputPropertyName +
                                 " is also an input property.");
      }
      if (ioMap.find(inputPropertyName) != ioMap.end()) {
        throw std::runtime_error("Cannot assign more than one output to " +
                                 inputPropertyName + '.');
      }
      ioMap[inputPropertyName] = outputPropertyName;
    }
  }
  // Declare order of execution and fill in propertyTable.
  cleanPropertyTable(propertyTable, ioMap);
  std::deque<size_t> queue;
  for (size_t i = 0; i < setupTable->rowCount(); ++i) {
    configureRow(setupTable, propertyTable, i, queue, ioMap);
  }

  // Execute the algorithm in the order specified by queue.
  const std::string algorithmName = getProperty(PropertyNames::ALGORITHM);
  auto &algorithmFactory = AlgorithmFactory::Instance();
  while (!queue.empty()) {
    const auto row = queue.front();
    auto algorithm = algorithmFactory.create(
        algorithmName, algorithmFactory.highestVersion(algorithmName));
    algorithm->initialize();
    if (!algorithm->isInitialized()) {
      throw std::runtime_error("Workflow algorithm failed to initialise.");
    }
    // First column in taskTable is for id.
    for (size_t col = 1; col < propertyTable->columnCount(); ++col) {
      const auto column = propertyTable->getColumn(col);
      const auto &propertyName = column->name();
      const auto &valueType = column->get_type_info();
      try {
        if (valueType == typeid(std::string)) {
          const auto &value = propertyTable->cell<std::string>(row, col);
          algorithm->setProperty(propertyName, value);
        } else if (valueType == typeid(int)) {
          const auto &value = propertyTable->cell<int>(row, col);
          algorithm->setProperty(propertyName, static_cast<long>(value));
        } else if (valueType == typeid(size_t)) {
          const auto &value = propertyTable->cell<size_t>(row, col);
          algorithm->setProperty(propertyName, value);
        } else if (valueType == typeid(float) || valueType == typeid(double)) {
          const auto &value = propertyTable->cell<double>(row, col);
          algorithm->setProperty(propertyName, value);
        } else if (valueType == typeid(bool)) {
          const auto &value = propertyTable->cell<bool>(row, col);
          algorithm->setProperty(propertyName, value);
        } else if (valueType == typeid(Kernel::V3D)) {
          const auto &value = propertyTable->cell<V3D>(row, col);
          algorithm->setProperty(propertyName, value);
        } else {
          throw std::runtime_error("Unimplemented column type in " +
                                   PropertyNames::SETUP_TABLE + ": " +
                                   valueType.name() + '.');
        }
      } catch (std::invalid_argument &e) {
        throw std::runtime_error("While setting properties for algorithm " +
                                 algorithmName + ": " + e.what());
      }
    }
    algorithm->execute();
    if (!algorithm->isExecuted()) {
      throw std::runtime_error("Workflow algorithm failed to execute.");
    }
    queue.pop_front();
  }
}

/**
 * Appends `currentRow` and the rows it depends on to `queue` if needed.
 * Additionally, configures `propertyTable` appropriately.
 * @tparam QUEUE a type which supports iterators and has `emplace_back()`
 * @tparam MAP a type with `std::map` like iterators
 * @param[in] setupTable a table defining the property dependencies
 * @param[in,out] propertyTable a table containing the final property values
 * @param[in] currentRow the row in `setupTable` to configure
 * @param[in,out] queue a queue of row indexes in order of execution
 * @param[in] ioMap input/output property lookup table
 * @param[in,out] rowsBeingQueued a set of rows currently being configured
 * @throw std::runtime_error in several errorneous cases
 */
template <typename QUEUE, typename MAP>
void WorkflowAlgorithmRunner::configureRow(
    ITableWorkspace_sptr setupTable, ITableWorkspace_sptr propertyTable,
    const size_t currentRow, QUEUE &queue, const MAP &ioMap,
    std::shared_ptr<std::unordered_set<size_t>> rowsBeingQueued) const {
  // This method works recursively with regards to dependency resolution.

  if (currentRow > setupTable->rowCount()) {
    throw std::runtime_error("Current row " + std::to_string(currentRow) +
                             " out of task table bounds " +
                             std::to_string(setupTable->rowCount()) + '.');
  }

  // 1. currentRow is already in queue? Nothing to be done, then.
  // Here we blindly assume that propertyTable has been configured as well.
  if (std::find(queue.cbegin(), queue.cend(), currentRow) != queue.cend()) {
    return;
  }

  // 2. Check if currentRow is being processed already to prevent circular
  // dependencies. If not, mark the row as such.
  if (!rowsBeingQueued) {
    rowsBeingQueued.reset(new std::unordered_set<size_t>());
  }
  const auto status = rowsBeingQueued->emplace(currentRow);
  if (!status.second) {
    throw std::runtime_error("Circular dependencies!");
  }

  // 3a. Find the rows on which currentRow depends on. For each of them,
  // call this method recursively to make sure they are in the queue
  // before currentRow.
  // 3b. Configure propertyTable for currentRow by adding the correct
  // workspace names to input/output columns.
  for (const auto &ioPair : ioMap) {
    const auto &outputId =
        setupTable->getRef<std::string>(ioPair.first, currentRow);
    if (!outputId.empty()) {
      if (isHardCodedWorkspaceName(outputId)) {
        // Handle hard-coded input.
        propertyTable->getRef<std::string>(ioPair.first, currentRow) =
            tidyWorkspaceName(outputId);
      } else {
        // If input is not hard-coded, we have a dependency.
        // Find the source row.
        size_t outputRow = -1;
        try {
          setupTable->find(outputId, outputRow, 0);
        } catch (std::out_of_range &) {
          throw std::runtime_error(
              "Identifier \"" + outputId + "\" not found in " +
              PropertyNames::SETUP_TABLE + " (referenced in row " +
              std::to_string(currentRow) + ", column \"" + ioPair.first +
              "\").");
        }
        // Configure the source row and recursively the rows it depends on.
        configureRow(setupTable, propertyTable, outputRow, queue, ioMap,
                     rowsBeingQueued);
        const auto outputCol = ioPair.second;
        auto outputWorkspaceName =
            setupTable->getRef<std::string>(outputCol, outputRow);
        if (outputWorkspaceName.empty()) {
          throw std::runtime_error("No source workspace name found for " +
                                   ioPair.first + '.');
        }
        // Handle forced output.
        if (isHardCodedWorkspaceName(outputWorkspaceName)) {
          outputWorkspaceName = tidyWorkspaceName(outputWorkspaceName);
        }
        propertyTable->getRef<std::string>(ioPair.first, currentRow) =
            outputWorkspaceName;
        propertyTable->getRef<std::string>(outputCol, outputRow) =
            outputWorkspaceName;
      }
    }
  }

  // 4. Dependencies have been taken care of -> add currentRow to
  // queue.
  queue.emplace_back(currentRow);

  // 5. Finally, unmark currentRow as being processed.
  rowsBeingQueued->erase(currentRow);
}

} // namespace Algorithms
} // namespace Mantid
