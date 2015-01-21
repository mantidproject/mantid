//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"

#include <iostream>

namespace Mantid {
namespace API {
using namespace Geometry;

/** Initialize the function providing it the workspace
 * @param workspace :: The workspace to set
 * @param wi :: The workspace index
 * @param startX :: The lower bin index
 * @param endX :: The upper bin index
 */
void IFunctionMW::setMatrixWorkspace(
    boost::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi,
    double startX, double endX) {
  m_workspace = workspace;
  m_workspaceIndex = wi;

  IFunction::setMatrixWorkspace(workspace, wi, startX, endX);
}

/**
 * Get a shared pointer to the saved matrix workspace.
 */
boost::shared_ptr<const API::MatrixWorkspace>
IFunctionMW::getMatrixWorkspace() const {
  return m_workspace.lock();
}

} // namespace API
} // namespace Mantid
