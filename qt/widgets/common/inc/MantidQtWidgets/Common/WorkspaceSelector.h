// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_WORKSPACESELECTOR_H_
#define MANTIDQTMANTIDWIDGETS_WORKSPACESELECTOR_H_

#include "DllOption.h"
#include "MantidAPI/AnalysisDataService.h"
#include <QComboBox>
#include <QStringList>

#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

// Forward declarations
namespace Mantid {
namespace API {
class Algorithm;
}
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {
/**
This class defines a widget for selecting a workspace present in the
AnalysisDataService

Subscribes to the WorkspaceAddNotification and WorkspaceDeleteNotification from
the ADS.

Types of workspace to show can be restricted in several ways:

 * By listing allowed WorkspaceIDs to show (Workspace2D, TableWorkspace, etc)
 * By deciding whether or not to show "hidden" workspaces (identified with a
double underscore at the start of the
          workspace name
 * By providing a suffix that the workspace name must have
 * By giving the name of an algorithm, each workspace will be validated as an
input to the workspaces first input WorkspaceProperty

@author Michael Whitty
@date 23/02/2011
*/
class EXPORT_OPT_MANTIDQT_COMMON WorkspaceSelector : public QComboBox {
  Q_OBJECT

  Q_PROPERTY(
      QStringList WorkspaceTypes READ getWorkspaceTypes WRITE setWorkspaceTypes)
  Q_PROPERTY(
      bool ShowHidden READ showHiddenWorkspaces WRITE showHiddenWorkspaces)
  Q_PROPERTY(bool ShowGroups READ showWorkspaceGroups WRITE showWorkspaceGroups)
  Q_PROPERTY(bool Optional READ isOptional WRITE setOptional)
  Q_PROPERTY(QStringList Suffix READ getSuffixes WRITE setSuffixes)
  Q_PROPERTY(QString Algorithm READ getValidatingAlgorithm WRITE
                 setValidatingAlgorithm)
  friend class DataSelector;

public:
  using QComboBox::currentIndexChanged;

  /// Default Constructor
  WorkspaceSelector(QWidget *parent = nullptr, bool init = true);
  /// Destructor
  ~WorkspaceSelector() override;

  QStringList getWorkspaceTypes() const;
  void setWorkspaceTypes(const QStringList &types);
  bool showHiddenWorkspaces() const;
  void showHiddenWorkspaces(bool show);
  bool showWorkspaceGroups() const;
  void showWorkspaceGroups(bool show);
  bool isOptional() const;
  void setOptional(bool optional);
  QStringList getSuffixes() const;
  void setSuffixes(const QStringList &suffix);
  void setLowerBinLimit(int numberOfBins);
  void setUpperBinLimit(int numberOfBins);
  QString getValidatingAlgorithm() const;
  void setValidatingAlgorithm(const QString &algName);
  bool isValid() const;
  void refresh();

signals:
  void emptied();

private:
  void handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf);
  void handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf);
  void handleClearEvent(Mantid::API::ClearADSNotification_ptr pNf);
  void handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf);
  void
  handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);

  bool checkEligibility(const QString &name,
                        Mantid::API::Workspace_sptr object) const;
  bool hasValidSuffix(const QString &name) const;
  bool hasValidNumberOfBins(Mantid::API::Workspace_sptr object) const;

protected:
  // Method for handling drop events
  void dropEvent(QDropEvent * /*unused*/) override;
  // called when a drag event enters the class
  void dragEnterEvent(QDragEnterEvent * /*unused*/) override;

private:
  /// Poco Observers for ADS Notifications
  Poco::NObserver<WorkspaceSelector, Mantid::API::WorkspaceAddNotification>
      m_addObserver;
  Poco::NObserver<WorkspaceSelector,
                  Mantid::API::WorkspacePostDeleteNotification>
      m_remObserver;
  Poco::NObserver<WorkspaceSelector, Mantid::API::ClearADSNotification>
      m_clearObserver;
  Poco::NObserver<WorkspaceSelector, Mantid::API::WorkspaceRenameNotification>
      m_renameObserver;
  Poco::NObserver<WorkspaceSelector,
                  Mantid::API::WorkspaceAfterReplaceNotification>
      m_replaceObserver;

  bool m_init;

  /// A list of workspace types that should be shown in the ComboBox
  QStringList m_workspaceTypes;
  /// Whether to show "hidden" workspaces
  bool m_showHidden;
  // show/hide workspace groups
  bool m_showGroups;
  /// Whether to add an extra empty entry to the combobox
  bool m_optional;
  /// Allows you to put limits on the size of the workspace i.e. number of bins
  std::pair<int, int> m_binLimits;
  QStringList m_suffix;
  QString m_algName;
  QString m_algPropName;

  // Algorithm to validate against
  boost::shared_ptr<Mantid::API::Algorithm> m_algorithm;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_
