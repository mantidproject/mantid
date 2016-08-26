#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H

#include <memory>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class DataProcessorCommand;

/** @class DataProcessorTreeManager

// TODO: Description

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

class DataProcessorTreeManager {
public:
  virtual ~DataProcessorTreeManager(){};

  /// Actions/commands

  /// Publish actions/commands
  virtual std::vector<std::unique_ptr<DataProcessorCommand>>
  publishCommands() = 0;

protected:
  /// Add a command to the list of available commands
  void addCommand(std::vector<std::unique_ptr<DataProcessorCommand>> &commands,
                  std::unique_ptr<DataProcessorCommand> command) {
    commands.push_back(std::move(command));
  }
};
}
}
#endif /* MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H */
