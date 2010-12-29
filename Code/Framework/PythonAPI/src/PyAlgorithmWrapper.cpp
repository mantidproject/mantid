//--------------------------------------------
// Includes
//--------------------------------------------
#include <MantidPythonAPI/PyAlgorithmWrapper.h>
#include <boost/python.hpp>

using namespace Mantid::PythonAPI;

//---------------------------------------------
// PyAlgorithmBase 
//---------------------------------------------
/**
 * Constructor
 */
PyAlgorithmBase::PyAlgorithmBase() : CloneableAlgorithm()
{
}

//--------------------------------------------------
// PyAlgorithmCallback
//--------------------------------------------------
/**
 * Constructor for callback
 * @param self The python object to use to perform the callback
 */
PyAlgorithmCallback::PyAlgorithmCallback(PyObject *self) : PyAlgorithmBase(), m_self(self), m_ref_killed(false)
{
  Py_INCREF(m_self);
}

/** 
 * Destructor
 */
PyAlgorithmCallback::~PyAlgorithmCallback()
{
  if( !m_ref_killed )
  {
    Py_XDECREF(m_self);
  }
}

/**
 * Return a clone of this object
 * @returns a pointer to the clone object
 */ 
Mantid::API::CloneableAlgorithm * PyAlgorithmCallback::clone()
{
  // Call the python clone method to get back a full copy of the whole object and
  // then return the C++ object extracted from this
  PyAlgorithmBase *cloned = PyCall_NoArg<PyAlgorithmBase*>::dispatch(m_self, "clone");
  return cloned;
}

/**
 * Call the name method on the Python object
 * @returns The name of the algorithm
 */ 
const std::string PyAlgorithmCallback::name() const 
{
  return PyCall_NoArg<const std::string>::dispatch(m_self, "name");
}

/**
 * Call the version method on the Python object
 * @returns The version number of the algorithm
 */
int PyAlgorithmCallback::version() const
{
  return PyCall_NoArg<const int>::dispatch(m_self, "version");
}

/**
 * Call the category method on the Python object
 * @returns The category this algorithm belongs to
 */
const std::string PyAlgorithmCallback::category() const 
{
  return PyCall_NoArg<const std::string>::dispatch(m_self, "category");
}

/**
 * Perform initialization for this algorithm. This just calls up to Python.
 */
void  PyAlgorithmCallback::init()
{
  PyCall_NoArg<void>::dispatch(m_self, "PyInit");
}

/**
 * Execute this algorithm. This just calls up to Python.
 */
void PyAlgorithmCallback::exec()
{
  PyCall_NoArg<void>::dispatch(m_self, "PyExec");
}
