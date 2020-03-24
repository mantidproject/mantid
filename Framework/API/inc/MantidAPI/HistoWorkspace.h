// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/** HistoWorkspace is an abstract base class for MatrixWorkspace types that are
  NOT event workspaces. This type has to exist as a helper for workspace
  creation: Many algorithms create a new MatrixWorkspace from a parent workspace
  without keeping the events, but keeping any potential sub type of
  MatrixWorkspace. HistoWorkspace provides a common base type for all non-event
  MatrixWorkspaces. See DataObjects/WorkspaceCreation.h for mor details.
*/
class MANTID_API_DLL HistoWorkspace : public MatrixWorkspace {
public:
  HistoWorkspace(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned)
      : MatrixWorkspace(storageMode) {}

  /// Returns a clone of the workspace
  std::unique_ptr<HistoWorkspace> clone() const {
    return std::unique_ptr<HistoWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<HistoWorkspace> cloneEmpty() const {
    return std::unique_ptr<HistoWorkspace>(doCloneEmpty());
  }

private:
  HistoWorkspace *doClone() const override = 0;
  HistoWorkspace *doCloneEmpty() const override = 0;
};

/// shared pointer to the HistoWorkspace class
using HistoWorkspace_sptr = std::shared_ptr<HistoWorkspace>;
/// shared pointer to a const HistoWorkspace
using HistoWorkspace_const_sptr = std::shared_ptr<const HistoWorkspace>;

} // namespace API
} // namespace Mantid
