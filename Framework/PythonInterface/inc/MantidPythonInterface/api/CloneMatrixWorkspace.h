#ifndef MANTID_PYTHONINTERFACE_CLONEMATRIXWORKSPACE_H_
#define MANTID_PYTHONINTERFACE_CLONEMATRIXWORKSPACE_H_
/*
  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include <boost/python/object.hpp> //Safer way to include Python.h
#include <vector>

namespace Mantid {
namespace API {
class MatrixWorkspace;
}

namespace PythonInterface {
//** @name Numpy clones of data*/
///{
/// Create a numpy array from the X values of the given workspace reference
PyObject *cloneX(API::MatrixWorkspace &self);
/// Create a numpy array from the Y values of the given workspace reference
PyObject *cloneY(API::MatrixWorkspace &self);
/// Create a numpy array from the E values of the given workspace reference
PyObject *cloneE(API::MatrixWorkspace &self);
/// Create a numpy array from the E values of the given workspace reference
PyObject *cloneDx(API::MatrixWorkspace &self);
///@}
}
}

#endif /* MANTID_PYTHONINTERFACE_CLONEMATRIXWORKSPACE_H_ */
