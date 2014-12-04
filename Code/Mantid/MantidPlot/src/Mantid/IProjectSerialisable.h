#ifndef MANTID_API_IPROJECTSERIALISABLE_H_
#define MANTID_API_IPROJECTSERIALISABLE_H_

#include <string>

class ApplicationWindow;

namespace Mantid
{

  /**
  Defines an interface to a MantidPlot class that can be saved into or loaded from a project.

  @author Harry Jeffery, ISIS, RAL
  @date 31/07/2014

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class IProjectSerialisable
  {
  public:
    /// Virtual destructor (required by linker on some versions of OS X/Intel compiler)
    virtual ~IProjectSerialisable() {}
    /// Loads the given lines from the project file and applies them.
    virtual void loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion) = 0;
    /// Serialises to a string that can be saved to a project file.
    virtual std::string saveToProject(ApplicationWindow* app) = 0;
  };

} // namespace Mantid

#endif  /* MANTID_API_IPROJECTSERIALISABLE_H_ */
