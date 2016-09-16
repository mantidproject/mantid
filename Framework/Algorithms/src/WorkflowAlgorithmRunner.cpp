#include "MantidAlgorithms/WorkflowAlgorithmRunner.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"

#include <unordered_map>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

namespace PropertyNames {
const static std::string ALGORITHM("Algorithm");
const static std::string IO_MAP("InputOutputMap");
const static std::string SETUP_TABLE("SetupTable");
}

bool isHardCodedWorkspaceName(const std::string &s) {
  if (s.size() < 3) {
    return false;
  }
  const auto f = s.front();
  const auto b = s.back();
  return (f == '\'' && b == '\'') || (f == '"' && b == '"');
}

std::string tidyWorkspaceName(const std::string &s) {
  if (s.length() < 3) {
    throw std::runtime_error("Workspace name is too short.");
  }
  return std::string(s.cbegin() + 1, s.cend() - 1);
}

template<typename MAP>
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

  declareProperty(PropertyNames::ALGORITHM, "", boost::make_shared<MandatoryValidator<std::string>>(), "Name of the algorithm to run");
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(
          PropertyNames::SETUP_TABLE.c_str(), "", Direction::Input),
      "Table workspace containing the setup of the runs.");
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(
          PropertyNames::IO_MAP.c_str(), "", Direction::Input),
      "Table workspace mapping algorithm outputs to inputs.");
}

void WorkflowAlgorithmRunner::exec() {
  ITableWorkspace_sptr setupTable = getProperty(PropertyNames::SETUP_TABLE);
  ITableWorkspace_sptr propertyTable = setupTable->clone();
  ITableWorkspace_sptr ioMappingTable = getProperty(PropertyNames::IO_MAP);
  if (ioMappingTable->rowCount() != 1) {
    throw std::runtime_error("Row count in " + PropertyNames::IO_MAP + " is " + std::to_string(ioMappingTable->rowCount()) + ", not 1.");
  }
  std::unordered_map<std::string, std::string> ioMap;
  const auto inputPropertyNames = ioMappingTable->getColumnNames();
  for (size_t col = 0; col < ioMappingTable->columnCount(); ++col) {
    const auto &inputPropertyName = ioMappingTable->getColumn(col)->name();
    const auto &outputPropertyName = ioMappingTable->String(0, col);
    if (!outputPropertyName.empty()) {
      if (std::find(inputPropertyNames.cbegin(), inputPropertyNames.cend(), outputPropertyName) != inputPropertyNames.cend()) {
        throw std::runtime_error("Property " + outputPropertyName + " linked to " + inputPropertyName + " is also an input property.");
      }
      if (ioMap.find(inputPropertyName) != ioMap.end()) {
        throw std::runtime_error("Cannot assign more than one output to " + inputPropertyName + '.');
      }
      ioMap[inputPropertyName] = outputPropertyName;
    }
  }
  cleanPropertyTable(propertyTable, ioMap);
  std::deque<size_t> queue;
  for (size_t i = 0; i < setupTable->rowCount(); ++i) {
    configureRow(setupTable, propertyTable, i, queue, ioMap);
  }

  const std::string algorithmName = getProperty(PropertyNames::ALGORITHM);
  while (!queue.empty()) {
    const auto row = queue.front();
    auto &algorithmFactory = AlgorithmFactory::Instance();
    auto algorithm = algorithmFactory.create(algorithmName, algorithmFactory.highestVersion(algorithmName));
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
        }
        else if (valueType == typeid(int)) {
          const auto &value = propertyTable->cell<int>(row, col);
          algorithm->setProperty(propertyName, static_cast<long>(value));
        }
        else if (valueType == typeid(size_t)) {
          const auto &value = propertyTable->cell<size_t>(row, col);
          algorithm->setProperty(propertyName, value);
        }
        else if (valueType == typeid(float) || valueType == typeid(double)) {
          const auto &value = propertyTable->cell<double>(row, col);
          algorithm->setProperty(propertyName, value);
        }
        else if (valueType == typeid(bool)) {
          const auto &value = propertyTable->cell<bool>(row, col);
          algorithm->setProperty(propertyName, value);
        }
        else if (valueType == typeid(Kernel::V3D)) {
          const auto &value = propertyTable->cell<V3D>(row, col);
          algorithm->setProperty(propertyName, value);
        }
        else {
          throw std::runtime_error("Unimplemented column type in " + PropertyNames::SETUP_TABLE + ": " + valueType.name() + '.');
        }
      }
      catch(std::invalid_argument &e) {
        throw std::runtime_error("While setting properties for algorithm " + algorithmName + ": " + e.what());
      }
    }
    algorithm->execute();
    if (!algorithm->isExecuted()) {
      throw std::runtime_error("Workflow algorithm failed to execute.");
    }
    queue.pop_front();
  }
}

template<typename QUEUE, typename MAP>
void WorkflowAlgorithmRunner::configureRow(ITableWorkspace_sptr setupTable, ITableWorkspace_sptr propertyTable, const size_t currentRow, QUEUE &queue, const MAP &ioMap, std::shared_ptr<std::unordered_set<size_t>> rowsBeingQueued) const {
  if (currentRow > setupTable->rowCount()) {
    throw std::runtime_error("Current row " + std::to_string(currentRow) + " out of task table bounds " + std::to_string(setupTable->rowCount()) + '.');
  }
  if (std::find(queue.cbegin(), queue.cend(), currentRow) != queue.cend()) {
    return;
  }
  if (!rowsBeingQueued) {
    rowsBeingQueued.reset(new std::unordered_set<size_t>());
  }
  const auto status = rowsBeingQueued->emplace(currentRow);
  if (!status.second) {
    // This row is already being processed.
    throw std::runtime_error("Circular dependencies!");
  }
  for (const auto &ioPair : ioMap) {
    const auto &outputId = setupTable->getRef<std::string>(ioPair.first, currentRow);
    if (!outputId.empty()) {
      if (isHardCodedWorkspaceName(outputId)) {
        propertyTable->getRef<std::string>(ioPair.first, currentRow) = tidyWorkspaceName(outputId);
      }
      else {
        size_t outputRow = -1;
        try {
          setupTable->find(outputId, outputRow, 0);
        }
        catch (std::out_of_range &) {
          throw std::runtime_error("Identifier \"" + outputId + "\" not found in " + PropertyNames::SETUP_TABLE + " (referenced in row " + std::to_string(currentRow) + ", column \"" + ioPair.first + "\").");
        }

        configureRow(setupTable, propertyTable, outputRow, queue, ioMap, rowsBeingQueued);
        const auto outputCol = ioPair.second;
        auto outputWorkspaceName = setupTable->getRef<std::string>(outputCol, outputRow);
        if (outputWorkspaceName.empty()) {
          throw std::runtime_error("No source workspace name found for " + ioPair.first + '.');
        }
        if (isHardCodedWorkspaceName(outputWorkspaceName)) {
          outputWorkspaceName = tidyWorkspaceName(outputWorkspaceName);
        }
        propertyTable->getRef<std::string>(ioPair.first, currentRow) = outputWorkspaceName;
        propertyTable->getRef<std::string>(outputCol, outputRow) = outputWorkspaceName;
      }
    }
  }
  queue.emplace_back(currentRow);
  rowsBeingQueued->erase(currentRow);
}

} // namespace Algorithms
} // namespace Mantid
