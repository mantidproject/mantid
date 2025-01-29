// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/Unit.h"

#ifndef Q_MOC_RUN
#include <memory>
#endif

namespace Mantid {

namespace API {

/** This is a specialization of IFunction for functions defined on a
   MatrixWorkspace.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009
*/
class MANTID_API_DLL IFunctionMW : public virtual IFunction {
public:
  /// Set MatrixWorkspace
  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                          double endX) override;
  /// Get shared pointer to the workspace
  std::shared_ptr<const API::MatrixWorkspace> getMatrixWorkspace() const;
  /// Get the workspace index
  size_t getWorkspaceIndex() const { return m_workspaceIndex; }

protected:
  /// Keep a weak pointer to the workspace
  std::weak_ptr<const API::MatrixWorkspace> m_workspace;
  /// An index to a spectrum
  size_t m_workspaceIndex = 0;
};

} // namespace API
} // namespace Mantid
