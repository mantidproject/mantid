// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_WORKSPACEPROVIDERNOTIFIABLE_H_
#define MANTID_MANTIDWIDGETS_WORKSPACEPROVIDERNOTIFIABLE_H_

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
  virtual ~WorkspaceProviderNotifiable() {}

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
#endif // MANTID_MANTIDWIDGETS_ADSNOTIFIABLE_H_