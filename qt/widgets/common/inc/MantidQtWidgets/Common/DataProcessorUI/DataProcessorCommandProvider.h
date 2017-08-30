#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDPROVIDER_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDPROVIDER_H
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TableAction.h"
#include "MantidQtWidgets/Common/DataProcessorUI/EditAction.h"
namespace MantidQt {
namespace MantidWidgets {
class EXPORT_OPT_MANTIDQT_COMMON DataProcessorCommandProvider {
public:
  using CommandIndex = int;
  using CommandIndices = std::vector<int>;
  using CommandVector = std::vector<std::unique_ptr<DataProcessorCommand>>;
  virtual const CommandVector &getTableCommands() const = 0;
  virtual CommandVector &getTableCommands() = 0;
  virtual CommandIndex indexOfCommand(TableAction action) const = 0;
  virtual CommandIndices getModifyingTableCommands() const = 0;

  virtual const CommandVector &getEditCommands() const = 0;
  virtual CommandVector &getEditCommands() = 0;

  virtual CommandIndex indexOfCommand(EditAction action) const = 0;
  virtual CommandIndices getPausingEditCommands() const = 0;
  virtual CommandIndices getProcessingEditCommands() const = 0;
  virtual CommandIndices getModifyingEditCommands() const = 0;
};
}
}
#endif // MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDPROVIDER_H
