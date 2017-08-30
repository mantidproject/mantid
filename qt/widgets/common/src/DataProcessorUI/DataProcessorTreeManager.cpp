#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorTreeManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCommand.h"

namespace MantidQt {
namespace MantidWidgets {

void DataProcessorTreeManager::addCommand(
    std::vector<std::unique_ptr<DataProcessorCommand>> &commands,
    std::unique_ptr<DataProcessorCommand> command) {
  commands.emplace_back(std::move(command));
}

typename DataProcessorTreeManager::CommandIndices
DataProcessorTreeManager::getModifyingCommands(const CommandVector &commands) {
  CommandIndices indices;
  for (auto i = 0; i < static_cast<int>(commands.size()); i++) {
    if (commands[i]->modifiesTable())
      indices.emplace_back(i);
  }
  return indices;
}

const typename DataProcessorTreeManager::CommandVector&
DataProcessorTreeManager::getTableCommands() const {
  return m_tableCommands;
}

typename DataProcessorTreeManager::CommandVector&
DataProcessorTreeManager::getTableCommands() {
  return m_tableCommands;
}

typename DataProcessorTreeManager::CommandVector&
DataProcessorTreeManager::getEditCommands() {
  return m_editCommands;
}

const typename DataProcessorTreeManager::CommandVector&
DataProcessorTreeManager::getEditCommands() const {
  return m_editCommands;
}

void DataProcessorTreeManager::addEditCommand(
    std::unique_ptr<DataProcessorCommand> command) {
  addCommand(m_editCommands, std::move(command));
}
void DataProcessorTreeManager::addTableCommand(
    std::unique_ptr<DataProcessorCommand> command) {
  addCommand(m_tableCommands, std::move(command));
}
}
}
