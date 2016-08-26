#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExpandCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGroupRowsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorImportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorNewTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOpenTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOptionsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPasteSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSeparatorCommand.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {

/**
* Constructor
* @param presenter :: a pointer to the presenter
*/
DataProcessorTwoLevelTreeManager::DataProcessorTwoLevelTreeManager(
    DataProcessorPresenter *presenter)
    : m_presenter(presenter) {}

/**
* Destructor
*/
DataProcessorTwoLevelTreeManager::~DataProcessorTwoLevelTreeManager() {}

/**
* Publishes a list of available commands
* @return : The list of available commands
*/
std::vector<DataProcessorCommand_uptr>
DataProcessorTwoLevelTreeManager::publishCommands() {

  std::vector<DataProcessorCommand_uptr> commands;

  addCommand(commands, make_unique<DataProcessorOpenTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorNewTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSaveTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSaveTableAsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorImportTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorExportTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorOptionsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorProcessCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorExpandCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPlotRowCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPlotGroupCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorAppendRowCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorAppendGroupCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorGroupRowsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorCopySelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorCutSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPasteSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorClearSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorDeleteRowCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorDeleteGroupCommand>(m_presenter));
  return commands;
}
}
}
