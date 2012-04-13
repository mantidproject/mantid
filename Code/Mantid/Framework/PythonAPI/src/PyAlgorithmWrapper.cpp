//--------------------------------------------
// Includes
//--------------------------------------------
#include "MantidPythonAPI/BoostPython_Silent.h"
#include "MantidPythonAPI/PyAlgorithmWrapper.h"
#include "MantidPythonAPI/PythonThreading.h"

using namespace Mantid::PythonAPI;

//---------------------------------------------
// PyAlgorithmBase 
//---------------------------------------------
/**
 * Constructor
 */
PyAlgorithmBase::PyAlgorithmBase() : Algorithm()
{
}

//--------------------------------------------------
// PyAlgorithmWrapper
//--------------------------------------------------
/**
 * Call the name method on the Python object
 * @returns The name of the algorithm
 */ 
const std::string PyAlgorithmWrapper::name() const
{
  GlobalInterpreterLock gil;
  PyObject *self = getSelf();
  return std::string(self->ob_type->tp_name);
}

/**
 * Call the version method on the Python object
 * @returns The version number of the algorithm
 */
int PyAlgorithmWrapper::version() const
{
  return PyCall_NoArg<const int>::dispatch(getSelf(), "version");
}

/**
 * Call the category method on the Python object
 * @returns The category this algorithm belongs to
 */
const std::string PyAlgorithmWrapper::category() const
{
  return PyCall_NoArg<const std::string>::dispatch(getSelf(), "category");
}

/**
 * Perform initialization for this algorithm. This just calls up to Python.
 */
void  PyAlgorithmWrapper::init()
{
  PyCall_NoArg<void>::dispatch(getSelf(), "PyInit");
}

/**
 * Execute this algorithm. This just calls up to Python.
 */
void PyAlgorithmWrapper::exec()
{
  PyCall_NoArg<void>::dispatch(getSelf(), "PyExec");
}

/// Returns the PyObject that owns this wrapper, i.e. self
/// @returns A pointer to self
PyObject * PyAlgorithmWrapper::getSelf() const
{
  return boost::python::detail::wrapper_base_::get_owner(*this);
}

