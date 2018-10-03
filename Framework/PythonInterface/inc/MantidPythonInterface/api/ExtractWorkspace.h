#ifndef MANTID_PYTHONINTERFACE_EXTRACTWORKSPACE_H_
#define MANTID_PYTHONINTERFACE_EXTRACTWORKSPACE_H_
/**
    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include <MantidAPI/Workspace.h>

#include <boost/python/object_fwd.hpp>

namespace Mantid {
namespace PythonInterface {

struct DLLExport ExtractWorkspace {
  ExtractWorkspace(const boost::python::object &pyvalue);
  bool check() const;
  const API::Workspace_sptr operator()() const;

private:
  API::Workspace_sptr m_value;
};
} // namespace PythonInterface
} // namespace Mantid

#endif // MANTID_PYTHONINTERFACE_EXTRACTWORKSPACE_H_
