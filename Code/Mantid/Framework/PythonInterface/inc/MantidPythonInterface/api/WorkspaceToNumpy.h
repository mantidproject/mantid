#ifndef MANTID_PYTHONINTERFACE_WORKSPACETONUMPY_H_
#define MANTID_PYTHONINTERFACE_WORKSPACETONUMPY_H_
/*
  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include <boost/python/object.hpp> //Safer way to include Python.h
#include <vector>

namespace Mantid
{
  namespace API
  {
    class MatrixWorkspace;
  }

  namespace PythonInterface
  {
    namespace Numpy
    {

#define DECLARE_ACCESS_FN(functionName) \
      PyObject *functionName(API::MatrixWorkspace &self, const size_t index);

      //** @name Numpy read-only wrappers */
      ///@{
      /// Create a read-only numpy wrapper around the original X values at the given index
      DECLARE_ACCESS_FN(readOnlyX);
      /// Create a read-only numpy wrapper around the original Y values at the given index
      DECLARE_ACCESS_FN(readOnlyY);
      /// Create a read-only numpy wrapper around the original X values at the given index
      DECLARE_ACCESS_FN(readOnlyE);
      /// Create a read-only numpy wrapper around the original Dx values at the given index
      DECLARE_ACCESS_FN(readOnlyDx);
      ///@}
      //** @name Numpy writable array wrappers */
      ///@{
      /// Create a writable wrapper around the original X values at the given index
      DECLARE_ACCESS_FN(readWriteX);
      /// Create a writable numpy wrapper around the original Y values at the given index
      DECLARE_ACCESS_FN(readWriteY);
      /// Create a writable numpy wrapper around the original X values at the given index
      DECLARE_ACCESS_FN(readWriteE);
      /// Create a writable numpy wrapper around the original Dx values at the given index
      DECLARE_ACCESS_FN(readWriteDx);
      ///@}
#undef DECLARE_ACCESS_FN

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
}


#endif /* MANTID_PYTHONINTERFACE_WORKSPACETONUMPY_H_ */
