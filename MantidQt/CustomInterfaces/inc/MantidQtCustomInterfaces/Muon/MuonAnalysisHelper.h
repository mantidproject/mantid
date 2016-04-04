#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISHELPER_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISHELPER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/DateAndTime.h"

#include <QSettings>
#include <QVector>
#include <QDoubleValidator>
#include <QLineEdit>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MuonAnalysisHelper
{

using namespace Mantid::API;
using namespace Mantid::Kernel;

/// Sets double validator for specified field
MANTIDQT_CUSTOMINTERFACES_DLL void setDoubleValidator(QLineEdit* field, bool allowEmpty = false);

/// Returns a first period MatrixWorkspace in a run workspace
MANTIDQT_CUSTOMINTERFACES_DLL MatrixWorkspace_sptr firstPeriod(Workspace_sptr ws);

/// Validates the field and returns the value
MANTIDQT_CUSTOMINTERFACES_DLL double getValidatedDouble(QLineEdit* field, const QString& defaultValue,
                                    const QString& valueDescr, Logger& log);

/// Returns a number of periods in a run workspace
MANTIDQT_CUSTOMINTERFACES_DLL size_t numPeriods(Workspace_sptr ws);

/// Print various information about the run
MANTIDQT_CUSTOMINTERFACES_DLL void printRunInfo(MatrixWorkspace_sptr runWs, std::ostringstream& out);

/// Get a run label for the workspace
MANTIDQT_CUSTOMINTERFACES_DLL std::string getRunLabel(const Workspace_sptr& ws);

/// Get a run label for a list of workspaces
MANTIDQT_CUSTOMINTERFACES_DLL std::string getRunLabel(std::vector<Workspace_sptr> wsList);

/// Sums a list of workspaces together
MANTIDQT_CUSTOMINTERFACES_DLL Workspace_sptr sumWorkspaces(const std::vector<Workspace_sptr>& workspaces);

/// Makes sure the specified workspaces are in specified group
MANTIDQT_CUSTOMINTERFACES_DLL void groupWorkspaces(const std::string& groupName,
                                                   const std::vector<std::string>& inputWorkspaces);

/// Finds runs of consecutive numbers
MANTIDQT_CUSTOMINTERFACES_DLL std::vector<std::pair<int, int>>
findConsecutiveRuns(const std::vector<int> &runs);

/// Replaces sample log value
MANTIDQT_CUSTOMINTERFACES_DLL void replaceLogValue(const std::string &wsName,
                                                   const std::string &logName,
                                                   const std::string &logValue);

/// Finds all of the values for a log
MANTIDQT_CUSTOMINTERFACES_DLL std::vector<std::string>
findLogValues(const Mantid::API::Workspace_sptr ws, const std::string &logName);

/// Finds the range of values for a log
MANTIDQT_CUSTOMINTERFACES_DLL std::pair<std::string, std::string> findLogRange(
    const Mantid::API::Workspace_sptr ws, const std::string &logName,
    bool (*isLessThan)(const std::string &first, const std::string &second));

/// Finds the range of values for a log for a vector of workspaces
MANTIDQT_CUSTOMINTERFACES_DLL std::pair<std::string, std::string> findLogRange(
    const std::vector<Mantid::API::Workspace_sptr> &workspaces,
    const std::string &logName,
    bool (*isLessThan)(const std::string &first, const std::string &second));

/// Concatenates time-series log of one workspace with the second
MANTIDQT_CUSTOMINTERFACES_DLL void
appendTimeSeriesLogs(boost::shared_ptr<Mantid::API::Workspace> toAppend,
                     boost::shared_ptr<Mantid::API::Workspace> resultant,
                     const std::string &logName);

/**
 * A class which deals with auto-saving the widget values. Widgets are
 * registered and then on any
 * change, their value is stored using QSettings.
 */
class MANTIDQT_CUSTOMINTERFACES_DLL WidgetAutoSaver : QObject
{
  Q_OBJECT

public:
  /// Constructor
  WidgetAutoSaver(const QString& groupName);

  /// Register new widget for auto-saving
  void registerWidget(QWidget* widget, const QString& name, QVariant defaultValue);

  /// Begin new auto-save group
  void beginGroup(const QString& name);

  /// End current auto-save group
  void endGroup();

  /// Enable/disable auto-saving of all the registered widgets
  void setAutoSaveEnabled(bool enabled);

  /// Enable/disable auto-saving of the given widget
  void setAutoSaveEnabled(QWidget* widget, bool enabled);

  /// Load the auto-saved (or default) value of all the registered widgets
  void loadWidgetValues();

  /// Load the auto-saved (or default) value of the given widget
  void loadWidgetValue(QWidget* widget);

private slots:
  /// Save the caller value
  void saveWidgetValue();

private:
  /// Return a signal (which can be used instead of SIGNAL()) which is emmited when given widget is changed
  const char* changedSignal(QWidget* widget);

  /// A list of all the registered widgets
  QVector<QWidget*> m_registeredWidgets;

  /// Names of registered widgets
  QMap<QWidget*, QString> m_widgetNames;

  /// Default values of registered widgets
  QMap<QWidget*, QVariant> m_widgetDefaultValues;

  /// Groups of registered widgets
  QMap<QWidget*, QString> m_widgetGroups;

  /// Settings used to keep track of the groups
  QSettings m_settings;
};

/// Validator which accepts valid doubles OR empty strings
class MANTIDQT_CUSTOMINTERFACES_DLL DoubleOrEmptyValidator : public QDoubleValidator
{
  Q_OBJECT

public:

  DoubleOrEmptyValidator(QObject* parent = NULL)
    : QDoubleValidator(parent)
  {}

  // See QValidator
  QValidator::State validate(QString &input, int &pos) const override {
    if (input.isEmpty())
      return QValidator::Acceptable;
    else
      return QDoubleValidator::validate(input,pos);
  }
};

} // namespace MuonAnalysisHelper
} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISHELPER_H_ */
