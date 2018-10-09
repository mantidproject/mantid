// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IWORKSPACEPROPERTY_H_
#define MANTID_API_IWORKSPACEPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace_fwd.h"

namespace Mantid {
namespace API {
/** An interface that is implemented by WorkspaceProperty.
    Used for non templated workspace operations.

    @author Nick Draper, Tessella Support Services plc
    @author Russell Taylor, Tessella Support Services plc
    @date 11/12/2007
*/
class IWorkspaceProperty {
public:
  /// Store a workspace into the AnalysisDataService
  virtual bool store() = 0;
  /// Clear the stored pointer
  virtual void clear() = 0;
  /// Get a pointer to the workspace
  virtual Workspace_sptr getWorkspace() const = 0;
  /// Is the input workspace property optional (can be blank)?
  virtual bool isOptional() const = 0;
  /// Will the workspace be locked when starting an algorithm?
  virtual bool isLocking() const = 0;
  /// Virtual destructor
  virtual ~IWorkspaceProperty() = default;
  /// Sets a flag indicating whether this is the master rank in MPI builds.
  virtual void setIsMasterRank(bool isMasterRank) = 0;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IWORKSPACEPROPERTY_H_*/
