// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/**
\class  WorkspaceProviderNotifiable
\brief  Mixin class allows ADS notifications
\author Lamar Moore
\date   24-08-2016
\version 1.0
*/
namespace MantidQt {
namespace MantidWidgets {

class WorkspaceProviderNotifiable {
public:
  virtual ~WorkspaceProviderNotifiable() = default;

  enum class Flag {
    WorkspaceLoaded,
    WorkspaceRenamed,
    WorkspaceDeleted,
    WorkspacesCleared,
    WorkspacesGrouped,
    WorkspacesUngrouped,
    WorkspaceGroupUpdated,
    GenericUpdateNotification
  };

  virtual void notifyFromWorkspaceProvider(Flag flag) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
