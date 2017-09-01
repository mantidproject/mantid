#include "MantidQtWidgets/Common/DataProcessorUI/CommandProviderBase.h"
namespace MantidQt {
namespace MantidWidgets {
class GenericTwoLevelCommandProvider : public CommandProviderBase {
public:
  GenericTwoLevelCommandProvider(GenericDataProcessorPresenter& presenter);
  CommandIndex indexOfCommand(TableAction action) const override;
  CommandIndices getModifyingTableCommands() const override;
  CommandIndex indexOfCommand(EditAction action) const override;
  CommandIndices getPausingEditCommands() const override;
  CommandIndices getProcessingEditCommands() const override;
  CommandIndices getModifyingEditCommands() const override;
private:
  void addTableCommands();
  void addEditCommands();
};
}
}
