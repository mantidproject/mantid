// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {
using namespace Geometry;

/** Initialize the function providing it the workspace
 * @param workspace :: The workspace to set
 * @param wi :: The workspace index
 * @param startX :: The lower bin index
 * @param endX :: The upper bin index
 */
void IFunctionMW::setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                                     double endX) {
  m_workspace = workspace;
  m_workspaceIndex = wi;

  IFunction::setMatrixWorkspace(workspace, wi, startX, endX);
}

/**
 * Get a shared pointer to the saved matrix workspace.
 */
std::shared_ptr<const API::MatrixWorkspace> IFunctionMW::getMatrixWorkspace() const { return m_workspace.lock(); }

} // namespace API
} // namespace Mantid
