#include "MantidPythonInterface/kernel/Environment/Threading.h"
#include <Poco/Thread.h>

namespace // <anonymous>
{
  /// The main thread state
  PyThreadState *g_mainThreadState = NULL;
}

namespace Mantid
{
namespace PythonInterface
{
namespace Environment
{

/// Saves a pointer to the PyThreadState of the main thread
void saveMainThreadState(PyThreadState *threadState)
{
  g_mainThreadState = threadState;
}

//-----------------------------------------------------------------------------
// PythonThreadState
//-----------------------------------------------------------------------------

PythonThreadState::PythonThreadState()
  : m_mainThreadState(NULL), m_thisThreadState(NULL)
{
  m_mainThreadState = g_mainThreadState;
  assert(m_mainThreadState);
  if(Poco::Thread::current())
  {
    PyEval_AcquireLock();
    PyInterpreterState * mainInterpreterState = m_mainThreadState->interp;
    m_thisThreadState = PyThreadState_New(mainInterpreterState);
    PyThreadState_Swap(m_thisThreadState);
  }
}

PythonThreadState::~PythonThreadState()
{
  if(m_thisThreadState)
  {
    PyThreadState_Swap(m_mainThreadState);
    PyThreadState_Clear(m_thisThreadState);
    PyThreadState_Delete(m_thisThreadState);
    PyEval_ReleaseLock();
  }
}

}
}
}
