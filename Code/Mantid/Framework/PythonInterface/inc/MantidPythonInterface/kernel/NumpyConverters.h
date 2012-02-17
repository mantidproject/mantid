#ifndef MANTID_PYTHONINTERFACE_NUMPYCONVERTERS_H_
#define MANTID_PYTHONINTERFACE_NUMPYCONVERTERS_H_
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
#include "MantidKernel/System.h"
#include <boost/python/object.hpp> //Safer way to include Python.h
#include <vector>
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Numpy
    {
      /** @name Create Numpy arrays */
      //@{
      /// Create a read-only array wrapper around a double Matrix
      DLLExport PyObject *wrapWithNumpy(const Kernel::DblMatrix & data);
      /// Create a read-only array wrapper around a double Matrix
      DLLExport PyObject *wrapWithReadOnlyNumpy(const Kernel::DblMatrix & data);
      //@}

      /** @name Create Mantid objects from python sequences */
      //@{
      /// Try and create a Mantid V3D object from the given PyObject.
      DLLExport Kernel::V3D createV3D(PyObject *data);
      /// Create a Matrix of doubles from a 2D numpy array
      DLLExport Kernel::DblMatrix createDoubleMatrix(PyObject* data);
      //@}

    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_NUMPYCONVERTERS_H_ */
