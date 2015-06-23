#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGCUSTOM_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGCUSTOM_H_

#include <string>

#include "MantidQtCustomInterfaces/Tomography/TomoRecToolConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Configuration of a third party tomographic reconstruction tool as a
Custom command line. To run a reconstruction tool via a command of the
user's choice with free-form options.

This is under development, and as it is not necessarily related to
custom interfaces might be moved out of here. Tools of other type
might be added, and then this should not be a sublcass of
TomoRecToolConfig but of a more general ToolConfig class.

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class ToolConfigCustom : public TomoRecToolConfig {
public:
  ToolConfigCustom() {}

  ToolConfigCustom(const std::string &runnable, const std::string &cmdOptions)
      : TomoRecToolConfig(runnable), m_opts(cmdOptions) {}

  ~ToolConfigCustom() {}

protected:
  virtual std::string makeCmdLineOptions() const { return m_opts; }

  virtual std::string makeExecutable() const { return m_runnable; };

private:
  std::string m_opts;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGCUSTOM_H_
