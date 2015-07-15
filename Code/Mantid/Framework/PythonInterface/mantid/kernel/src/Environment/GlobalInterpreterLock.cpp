#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"

namespace Mantid {
namespace PythonInterface {
namespace Environment {

//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------
/**
 * Calls PyGILState_Ensure.
 */
GlobalInterpreterLock::GlobalInterpreterLock() : m_state(PyGILState_Ensure()) {
}

/**
 * Calls PyGILState_Release.
 */
GlobalInterpreterLock::~GlobalInterpreterLock() {
  PyGILState_Release(m_state);
}

}
}
}
