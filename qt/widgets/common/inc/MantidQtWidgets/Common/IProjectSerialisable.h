#ifndef MANTID_API_IProjectSerialisable_H_
#define MANTID_API_IProjectSerialisable_H_

#include "MantidKernel/System.h"
#include <stdexcept>
#include <string>
#include <vector>

class ApplicationWindow;

namespace MantidQt {
namespace API {

/**
Defines an interface to a MantidPlot class that can be saved into or loaded from
a project.

@author Harry Jeffery, ISIS, RAL
@date 31/07/2014

Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class IProjectSerialisable {
public:
  /// Virtual destructor (required by linker on some versions of OS X/Intel
  /// compiler)
  virtual ~IProjectSerialisable() {}

  static IProjectSerialisable *loadFromProject(const std::string &lines,
                                               ApplicationWindow *app,
                                               const int fileVersion) {
    UNUSED_ARG(lines);
    UNUSED_ARG(app);
    UNUSED_ARG(fileVersion);
    throw std::runtime_error("Not implemented");
  }

  /// Serialises to a string that can be saved to a project file.
  virtual std::string saveToProject(ApplicationWindow *app) = 0;
  /// Returns a list of workspace names that are used by this window
  virtual std::vector<std::string> getWorkspaceNames() = 0;
  /// Returns the user friendly name of the window
  virtual std::string getWindowName() = 0;
  /// Returns the type of the window
  virtual std::string getWindowType() = 0;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_ProjectSerialisable_H_ */
