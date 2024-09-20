// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/AnalysisDataService.h"
#include <QStringList>
#include <QTableWidget>
#include <mutex>
#include <optional>

#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

typedef std::vector<std::pair<std::string, std::string>> StringPairVec;
namespace MantidQt {
namespace MantidWidgets {
/**
This class defines a widget for selecting multiple workspace present in the
AnalysisDataService and adding to a QTableWidget.

Subscribes to the WorkspaceAddNotification and WorkspaceDeleteNotification from
the ADS.

Types of workspace to show can be restricted in several ways:

 * By listing allowed WorkspaceIDs to show (Workspace2D, TableWorkspace, etc)
 * By deciding whether or not to show "hidden" workspaces (identified with a
double underscore at the start of the
          workspace name
 * By providing a suffix that the workspace name must have
*/
class EXPORT_OPT_MANTIDQT_COMMON WorkspaceMultiSelector final : public QTableWidget {
  Q_OBJECT

  Q_PROPERTY(QStringList Suffix READ getWSSuffixes WRITE setWSSuffixes)

public:
  /// Default Constructor
  explicit WorkspaceMultiSelector(QWidget *parent = nullptr);

  /// Destructor
  ~WorkspaceMultiSelector() override;

  void setupTable();

  void refresh();
  void resetIndexRangeToDefault();
  void unifyRange();

  StringPairVec retrieveSelectedNameIndexPairs();

  QStringList getWSSuffixes() const;
  void setWSSuffixes(const QStringList &suffix);

  bool isValid() const;

  void connectObservers() const;
  void disconnectObservers() const;

signals:
  void emptied();
  void focussed();

private:
  void handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf);
  void handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf);
  void handleClearEvent(Mantid::API::ClearADSNotification_ptr pNf);
  void handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf);
  void handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);

  bool checkEligibility(const std::string &name) const;
  bool hasValidSuffix(const std::string &name) const;

  void addItem(const std::string &name);
  void addItems(const std::vector<std::string> &names);
  void renameItem(const std::string &newName, int row);

protected:
  // Method for handling focus in events
  void focusInEvent(QFocusEvent * /*unused*/) override;

private:
  /// Poco Observers for ADS Notifications
  Poco::NObserver<WorkspaceMultiSelector, Mantid::API::WorkspaceAddNotification> m_addObserver;
  Poco::NObserver<WorkspaceMultiSelector, Mantid::API::WorkspacePostDeleteNotification> m_remObserver;
  Poco::NObserver<WorkspaceMultiSelector, Mantid::API::ClearADSNotification> m_clearObserver;
  Poco::NObserver<WorkspaceMultiSelector, Mantid::API::WorkspaceRenameNotification> m_renameObserver;
  Poco::NObserver<WorkspaceMultiSelector, Mantid::API::WorkspaceAfterReplaceNotification> m_replaceObserver;

  QStringList m_suffix;
  // Mutex for synchronized event handling
  std::mutex m_adsMutex;
};
} // namespace MantidWidgets
} // namespace MantidQt
