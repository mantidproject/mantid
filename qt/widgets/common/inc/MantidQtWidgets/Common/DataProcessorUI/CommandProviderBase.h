#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDPROVIDERBASE_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDPROVIDERBASE_H
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCommandProvider.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <MantidKernel/make_unique.h>
namespace MantidQt {
namespace MantidWidgets {
/** @class CommandProviderBase

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_COMMON CommandProviderBase : public DataProcessorCommandProvider {
public:
  CommandProviderBase(DataProcessorPresenter &presenter);
  CommandProviderBase(CommandProviderBase&&) = delete;
  CommandProviderBase& operator=(CommandProviderBase&&) = delete;
  
  const CommandVector &getTableCommands() const override;
  CommandVector &getTableCommands();// override;

  const CommandVector &getEditCommands() const override;
  CommandVector &getEditCommands();// override;

protected:
  DataProcessorPresenter &getPresenter();
  void addEditCommand(std::unique_ptr<DataProcessorCommand> command);
  void addTableCommand(std::unique_ptr<DataProcessorCommand> command);

  static CommandIndices getModifyingCommands(const CommandVector &commands);

  template <typename Command> void addEditCommand() {
    addEditCommand(::Mantid::Kernel::make_unique<Command>(&m_presenter));
  }

  template <typename Command> void addTableCommand() {
    addTableCommand(::Mantid::Kernel::make_unique<Command>(&m_presenter));
  }

private:
  DataProcessorPresenter &m_presenter;
  CommandVector m_editCommands;
  CommandVector m_tableCommands;
};
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDPROVIDERBASE_H
