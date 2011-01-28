//-------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------
#include "MantidPythonAPI/MantidVecHelper.h"
#include <numpy/arrayobject.h>

namespace Mantid
{
  namespace PythonAPI
  {
    /// Flag whether numpy should be used in wrapping a MantidVec  
    bool MantidVecHelper::g_useNumPy = false;
    /// Flag if initialize has been called already
    bool MantidVecHelper::g_isInitialized = false;

    /**
     * Try and initialize numpy module, setting the useNumPy flag appropriately
     * Note that this can only be called once and must be called from the init section of the Python module
     */
    void MantidVecHelper::initializeDependencies()
    {
      if( MantidVecHelper::g_isInitialized ) return;
      // Import numpy and check it succeeded
      int result = _import_array();
      if( result < 0 )
      {
        PyErr_Clear();
        MantidVecHelper::g_useNumPy = false;
      }
      else
      {
        MantidVecHelper::g_useNumPy = true;
      }

      MantidVecHelper::g_isInitialized = true;
    }

    /**
    * Creates an approriate wrapper for the MantidVec array, i.e. numpy array if it is available or Python list if not
    * @param values :: A reference to the MantidVec
    * @param readonly :: If true the array is flagged as read only (only used for numpy arrays)
    * @returns A pointer to a PyObject that contains the data
    */
    PyObject * MantidVecHelper::createPythonWrapper(const MantidVec & values, bool readonly)
    {
      if( g_useNumPy )
      {
        return MantidVecHelper::createNumPyArray(values, readonly);
      }
      else
      {
        return MantidVecHelper::createPythonList(values);
      }
    }

    /**
    * Create a NumPy wrapper around the given values and marks it as read only
    * @param values :: A reference to the array of values that will be wrapped by NumPy
    * @param readonly :: If true the array is flagged as read only
    * @returns A numpy wrapped array C-array
    */
    PyObject * MantidVecHelper::createNumPyArray(const MantidVec & values, bool readonly)
    {
      npy_intp dims[1] = { values.size() };
      PyArrayObject * ndarray = 
        (PyArrayObject*)PyArray_SimpleNewFromData(1, dims, NPY_DOUBLE,(void*)&(values[0]));
      if( readonly )
      {
        ndarray->flags &= ~NPY_WRITEABLE;
      }
      return (PyObject*)ndarray;
    }

    /**
    * Creates a Python list from a Mantid vector
    * @param values :: A reference to the MantidVec
    * @returns A standard Python list object
    */
    PyObject * MantidVecHelper::createPythonList(const MantidVec & values)
    {
      const Py_ssize_t nVals = values.size();
      PyObject * pyValues = PyList_New(nVals);
      if( !pyValues )
      {
        throw std::runtime_error("Cannot create Python list from Mantid vector.");
      }
      MantidVec::const_iterator iend = values.end();
      Py_ssize_t index(0);
      for( MantidVec::const_iterator itr = values.begin(); itr != iend; ++itr )
      {
        PyList_SET_ITEM(pyValues, index, PyFloat_FromDouble((*itr)));
        ++index;
      }
      return pyValues;
    }

  }
}
