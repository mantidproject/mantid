#include "MantidQtWidgets/Common/DataProcessorUI/CommandProviderBase.h"
namespace MantidQt {
namespace MantidWidgets {
CommandProviderBase::CommandProviderBase(
    DataProcessorPresenter &presenter)
    : m_presenter(presenter) {}

DataProcessorPresenter &CommandProviderBase::getPresenter() {
  return m_presenter;
}

void CommandProviderBase::addTableCommand(
    std::unique_ptr<DataProcessorCommand> command) {
  m_tableCommands.emplace_back(std::move(command));
}

void CommandProviderBase::addEditCommand(
    std::unique_ptr<DataProcessorCommand> command) {
  m_editCommands.emplace_back(std::move(command));
}

const typename CommandProviderBase::CommandVector &
CommandProviderBase::getTableCommands() const {
  return m_tableCommands;
}

typename CommandProviderBase::CommandVector &
CommandProviderBase::getTableCommands() {
  return m_tableCommands;
}

typename CommandProviderBase::CommandVector &
CommandProviderBase::getEditCommands() {
  return m_editCommands;
}

const typename CommandProviderBase::CommandVector &
CommandProviderBase::getEditCommands() const {
  return m_editCommands;
}

typename CommandProviderBase::CommandIndices
CommandProviderBase::getModifyingCommands(const CommandVector &commands) {
  CommandIndices indices;
  for (auto i = 0; i < static_cast<int>(commands.size()); i++) {
    if (commands[i]->modifiesTable())
      indices.emplace_back(i);
  }
  return indices;
}
}
}
