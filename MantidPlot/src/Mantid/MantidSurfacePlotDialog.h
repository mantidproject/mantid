#ifndef MANTIDSURFACEPLOTDIALOG_H_
#define MANTIDSURFACEPLOTDIALOG_H_

#include <QComboBox>
#include "MantidWSIndexDialog.h"

/**
 * The MantidSurfacePlotDialog offers the same functionality of choosing a
 * workspace index/spectrum No as MantidWSIndexDialog, but adds to it the
 * ability to choose a log value and the name for an axis.
 * This is for plotting a surface from a WorkspaceGroup.
 *
 * - The user may choose only one spectrum No, not a range.
 * - The user is offered the choice of only those logs that have single values
 * per workspace.
 */
class MantidSurfacePlotDialog : public QDialog {
  Q_OBJECT

public:
  /// Struct to hold user input
  struct UserInputSurface {
    bool accepted;
    int plotIndex;
    QString axisName;
    QString logName;
    std::set<double> customLogValues;
  };

  /// Constructor - same parameters as one of the parent constructors, along
  /// with a list of the names of workspaces to be plotted.
  MantidSurfacePlotDialog(MantidUI *parent, Qt::WFlags flags,
                          QList<QString> wsNames, const QString &plotType);
  /// Returns a structure holding all of the selected options
  UserInputSurface getSelections() const;
  /// Returns the workspace index to be plotted
  int getPlot() const;
  /// Display an error message box
  static void showPlotOptionsError(const QString &message);
  /// The string "Workspace index"
  static const QString WORKSPACE_INDEX;
  /// The string "Custom"
  static const QString CUSTOM;

private slots:
  /// Called when the OK button is pressed.
  void plot();
  /// Called when the log selection is changed.
  void onLogSelected(const QString &logName);

private:
  MantidWSIndexWidget m_widget;
  /// Initializes the layout of the dialog
  void init(const QString &plotType);
  /// Initializes the layout of the log options
  void initLogs();
  /// Initializes the layout of the buttons
  void initButtons();
  /// Populate log combo box with log options
  void populateLogComboBox();
  /// Gets input name for log axis
  const QString getAxisName() const;
  /// Gets input name for log to use
  const QString getLogName() const;
  /// Returns the input custom log values
  const std::set<double> getCustomLogValues() const;
  /// A pointer to the parent MantidUI object
  MantidUI *m_mantidUI;
  /// A list of names of workspaces which are to be plotted.
  QList<QString> m_wsNames;
  /// Set to true when user accepts input
  bool m_accepted;
  /// Qt objects
  QPushButton *m_okButton, *m_cancelButton;
  QHBoxLayout *m_buttonBox;
  QVBoxLayout *m_logBox, *m_outer;
  QComboBox *m_logSelector;
  QLineEdit *m_axisNameEdit, *m_logValues;
  QLabel *m_logLabel, *m_axisLabel, *m_customLogLabel;
  /// Minimum width for dialog to fit title in
  static const int MINIMUM_WIDTH;
};

#endif
