// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISHELPER_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISHELPER_H_

#include "DllConfig.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

#include <QDoubleValidator>
#include <QLineEdit>
#include <QSettings>
#include <QVector>

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {
/// Types of entities we are dealing with
enum ItemType { Pair, Group };

/// Possible plot types users might request
enum PlotType { Asymmetry, Counts, Logarithm };

/// Parameters from parsed workspace name
struct DatasetParams {
  std::string label;
  std::string instrument;
  std::vector<int> runs;
  ItemType itemType;
  std::string itemName;
  PlotType plotType;
  std::string periods;
  size_t version;
};
/// Whether multiple fitting is enabled or disabled
enum class MultiFitState { Enabled, Disabled };
} // namespace Muon

namespace MuonAnalysisHelper {
/// Sets double validator for specified field
MANTIDQT_MUONINTERFACE_DLL void setDoubleValidator(QLineEdit *field,
                                                   bool allowEmpty = false);

/// Returns a first period MatrixWorkspace in a run workspace
MANTIDQT_MUONINTERFACE_DLL Mantid::API::MatrixWorkspace_sptr
firstPeriod(Mantid::API::Workspace_sptr ws);

/// Validates the field and returns the value
MANTIDQT_MUONINTERFACE_DLL double
getValidatedDouble(QLineEdit *field, const QString &defaultValue,
                   const QString &valueDescr, Mantid::Kernel::Logger &log);

/// Returns a number of periods in a run workspace
MANTIDQT_MUONINTERFACE_DLL size_t numPeriods(Mantid::API::Workspace_sptr ws);

/// Print various information about the run
MANTIDQT_MUONINTERFACE_DLL void
printRunInfo(Mantid::API::MatrixWorkspace_sptr runWs, std::ostringstream &out);

/// Get a run label for the workspace
MANTIDQT_MUONINTERFACE_DLL std::string
getRunLabel(const Mantid::API::Workspace_sptr &ws);

/// Get a run label for a list of workspaces
MANTIDQT_MUONINTERFACE_DLL std::string
getRunLabel(const std::vector<Mantid::API::Workspace_sptr> &wsList);

MANTIDQT_MUONINTERFACE_DLL bool isNumber(const QString &string);
/// Get a run label given instrument and run numbers
MANTIDQT_MUONINTERFACE_DLL std::string
getRunLabel(const std::string &instrument, const std::vector<int> &runNumbers);

/// Sums a list of workspaces together
MANTIDQT_MUONINTERFACE_DLL Mantid::API::Workspace_sptr
sumWorkspaces(const std::vector<Mantid::API::Workspace_sptr> &workspaces);

/// Makes sure the specified workspaces are in specified group
MANTIDQT_MUONINTERFACE_DLL void
groupWorkspaces(const std::string &groupName,
                const std::vector<std::string> &inputWorkspaces);

/// Finds runs of consecutive numbers
MANTIDQT_MUONINTERFACE_DLL std::vector<std::pair<int, int>>
findConsecutiveRuns(const std::vector<int> &runs);

/// Replaces sample log value
MANTIDQT_MUONINTERFACE_DLL void replaceLogValue(const std::string &wsName,
                                                const std::string &logName,
                                                const std::string &logValue);

/// Finds all of the values for a log
MANTIDQT_MUONINTERFACE_DLL std::vector<std::string>
findLogValues(const Mantid::API::Workspace_sptr ws, const std::string &logName);

/// Finds the range of values for a log
MANTIDQT_MUONINTERFACE_DLL std::pair<std::string, std::string> findLogRange(
    const Mantid::API::Workspace_sptr ws, const std::string &logName,
    bool (*isLessThan)(const std::string &first, const std::string &second));

/// Finds the range of values for a log for a vector of workspaces
MANTIDQT_MUONINTERFACE_DLL std::pair<std::string, std::string> findLogRange(
    const std::vector<Mantid::API::Workspace_sptr> &workspaces,
    const std::string &logName,
    bool (*isLessThan)(const std::string &first, const std::string &second));

/// Concatenates time-series log of one workspace with the second
MANTIDQT_MUONINTERFACE_DLL void
appendTimeSeriesLogs(boost::shared_ptr<Mantid::API::Workspace> toAppend,
                     boost::shared_ptr<Mantid::API::Workspace> resultant,
                     const std::string &logName);

/// Parse analysis workspace name
MANTIDQT_MUONINTERFACE_DLL MantidQt::CustomInterfaces::Muon::DatasetParams
parseWorkspaceName(const std::string &wsName);

/// Generate new analysis workspace name
MANTIDQT_MUONINTERFACE_DLL std::string generateWorkspaceName(
    const MantidQt::CustomInterfaces::Muon::DatasetParams &params);

/// Get "run: period" string from workspace name
MANTIDQT_MUONINTERFACE_DLL QString
runNumberString(const std::string &workspaceName, const std::string &firstRun);

/// Decide if grouping needs to be reloaded
MANTIDQT_MUONINTERFACE_DLL bool isReloadGroupingNecessary(
    const boost::shared_ptr<Mantid::API::Workspace> currentWorkspace,
    const boost::shared_ptr<Mantid::API::Workspace> loadedWorkspace);

/// Parse run label into instrument and runs
MANTIDQT_MUONINTERFACE_DLL void parseRunLabel(const std::string &label,
                                              std::string &instrument,
                                              std::vector<int> &runNumbers);

/// Get colors for workspaces to go in table
MANTIDQT_MUONINTERFACE_DLL QMap<int, QColor> getWorkspaceColors(
    const std::vector<boost::shared_ptr<Mantid::API::Workspace>> &workspaces);

/**
 * A class which deals with auto-saving the widget values. Widgets are
 * registered and then on any
 * change, their value is stored using QSettings.
 */
class MANTIDQT_MUONINTERFACE_DLL WidgetAutoSaver : QObject {
  Q_OBJECT

public:
  /// Constructor
  WidgetAutoSaver(const QString &groupName);

  /// Register new widget for auto-saving
  void registerWidget(QWidget *widget, const QString &name,
                      QVariant defaultValue);

  /// Begin new auto-save group
  void beginGroup(const QString &name);

  /// End current auto-save group
  void endGroup();

  /// Enable/disable auto-saving of all the registered widgets
  void setAutoSaveEnabled(bool enabled);

  /// Enable/disable auto-saving of the given widget
  void setAutoSaveEnabled(QWidget *widget, bool enabled);

  /// Load the auto-saved (or default) value of all the registered widgets
  void loadWidgetValues();

  /// Load the auto-saved (or default) value of the given widget
  void loadWidgetValue(QWidget *widget);

private slots:
  /// Save the caller value
  void saveWidgetValue();

private:
  /// Return a signal (which can be used instead of SIGNAL()) which is emmited
  /// when given widget is changed
  const char *changedSignal(QWidget *widget);

  /// A list of all the registered widgets
  QVector<QWidget *> m_registeredWidgets;

  /// Names of registered widgets
  QMap<QWidget *, QString> m_widgetNames;

  /// Default values of registered widgets
  QMap<QWidget *, QVariant> m_widgetDefaultValues;

  /// Groups of registered widgets
  QMap<QWidget *, QString> m_widgetGroups;

  /// Settings used to keep track of the groups
  QSettings m_settings;
};

/// Validator which accepts valid doubles OR empty strings
class MANTIDQT_MUONINTERFACE_DLL DoubleOrEmptyValidator
    : public QDoubleValidator {
  Q_OBJECT

public:
  DoubleOrEmptyValidator(QObject *parent = nullptr)
      : QDoubleValidator(parent) {}

  // See QValidator
  QValidator::State validate(QString &input, int &pos) const override {
    if (input.isEmpty())
      return QValidator::Acceptable;
    else
      return QDoubleValidator::validate(input, pos);
  }
};

} // namespace MuonAnalysisHelper
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISHELPER_H_ */
