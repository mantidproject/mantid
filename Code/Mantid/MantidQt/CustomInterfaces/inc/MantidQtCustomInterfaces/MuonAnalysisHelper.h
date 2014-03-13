#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISHELPER_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISHELPER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"

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
DLLExport void setDoubleValidator(QLineEdit* field, bool allowEmpty = false);

/// Returns a first period MatrixWorkspace in a run workspace
DLLExport MatrixWorkspace_sptr firstPeriod(Workspace_sptr ws);

/// Validates the field and returns the value
DLLExport double getValidatedDouble(QLineEdit* field, const QString& defaultValue,
                                    const QString& valueDescr, Logger& log);

/// Returns a number of periods in a run workspace
DLLExport size_t numPeriods(Workspace_sptr ws);

/// Print various information about the run
DLLExport void printRunInfo(MatrixWorkspace_sptr runWs, std::ostringstream& out);

/// Get a run label for the workspace
DLLExport std::string getRunLabel(const Workspace_sptr& ws);

/// Get a run label for a list of workspaces
DLLExport std::string getRunLabel(std::vector<Workspace_sptr> wsList);

/// Sums a list of workspaces together
DLLExport Workspace_sptr sumWorkspaces(const std::vector<Workspace_sptr>& workspaces);

/**
 * A class which deals with auto-saving the widget values. Widgets are registered and then on any
 * change, their value is stored using QSettings.
 */
class Q_DECL_EXPORT WidgetAutoSaver : QObject
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
class Q_DECL_EXPORT DoubleOrEmptyValidator : public QDoubleValidator
{
  Q_OBJECT

public:

  DoubleOrEmptyValidator(QObject* parent = NULL)
    : QDoubleValidator(parent)
  {}

  // See QValidator
  virtual QValidator::State validate(QString& input, int& pos) const
  {
    if (input.isEmpty())
      return QValidator::Acceptable;
    else
      return QDoubleValidator::validate(input,pos);
  }
};

} // namespace MuonAnalysisHelper
} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_MUONANALYSISHELPER_H_ */
