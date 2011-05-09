#ifndef MANTID_PYTHONAPI_MANTIDVECHELPER_H_
#define MANTID_PYTHONAPI_MANTIDVECHELPER_H_

//-----------------------------------
// Includes
//-----------------------------------
#include "MantidAPI/MatrixWorkspace.h"  // For MantidVec typedef
#include "MantidPythonAPI/BoostPython_Silent.h"
#include <MantidGeometry/Math/Matrix.h>// For Matrix

namespace Mantid
{
  namespace PythonAPI
  {
    /** 
    Wrappings for a MantidVec. 
    Added wrapping for Matrix.

    If numpy is available, the C-array underlying a MantidVec is wrapped in a read-only numpy.ndarray object.
    This provides a read-only view of the data while not having the overhead of copying it to Python. If numpy
    is unavailable then the a standard Python list is created and the data is copied into this list.

    @author ISIS, STFC
    @date 18/08/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    */

    struct MantidVecHelper
    {
      /// Intialize external module dependencies
      static void initializeDependencies();
      /// Choose an appropriate wrapping for a MantidVec
      static PyObject * createPythonWrapper(const MantidVec & values, bool readonly);
      /// Create a numpy array using the already allocated data
      static PyObject * createNumPyArray(const MantidVec & values, bool readonly);
      /// Create a standard Python list from the exising data
      static PyObject * createPythonList(const MantidVec & values);
      /// A flag marking if numpy should be used when wrapping the vector
      static bool g_useNumPy;
      /// Flag if the init routine has been called already
      static bool g_isInitialized;
      /// Choose an appropriate wrapping for a Matrix
      static PyObject * createPythonWrapper(const Geometry::MantidMat & values, bool readonly);
      /// Create a numpy array using the already allocated data for Matrix
      static PyObject * createNumPyArray(const Geometry::MantidMat & values, bool readonly);
      /// Import a Matrix from a 2D numpy array
      const Geometry::MantidMat getMatrixFromArray(boost::python::numeric::array& array);
    };
  }
}

#endif //MANTID_PYTHONAPI_MANTIDVECHELPER_H_
