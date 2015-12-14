#ifndef MANTIDSURFACEPLOTDIALOG_H_
#define MANTIDSURFACEPLOTDIALOG_H_

#include <QComboBox>
#include "MantidWSIndexDialog.h"

/**
 * The MantidSurfacePlotDialog offers the same functionality of choosing a
 * workspace index/spectrum ID as MantidWSIndexDialog, but adds to it the
 * ability to choose a log value and the name for an axis.
 * This is for plotting a surface from a WorkspaceGroup.
 *
 * - The user may choose only one spectrum ID, not a range.
 * - The user is offered the choice of only those logs that have single values
 * per workspace.
 */
class MantidSurfacePlotDialog : public QDialog {
  Q_OBJECT

public:
  /// Struct to hold user input
  struct UserInputSurface {
    MantidWSIndexWidget::UserInput plotOptions;
    QString axisName;
    QString logName;
  };

  /// Constructor - same parameters as one of the parent constructors, along
  /// with a list of the names of workspaces to be plotted.
  MantidSurfacePlotDialog(MantidUI *parent, Qt::WFlags flags,
                          QList<QString> wsNames);
  /// Returns a structure holding all of the selected options
  UserInputSurface getSelections() const;
  /// Returns the QMultiMap that contains all the workspaces that are to be
  /// plotted, mapped to the set of workspace indices.
  QMultiMap<QString, std::set<int>> getPlots() const;
private slots:
  /// Called when the OK button is pressed.
  void plot();

private:
  MantidWSIndexWidget m_widget;
  /// Initializes the layout of the dialog
  void init();
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
  /// A pointer to the parent MantidUI object
  MantidUI *m_mantidUI;
  /// A list of names of workspaces which are to be plotted.
  QList<QString> m_wsNames;
  /// Qt objects
  QPushButton *m_okButton, *m_cancelButton;
  QHBoxLayout *m_buttonBox;
  QVBoxLayout *m_logBox, *m_outer;
  QComboBox *m_logSelector;
  QLineEdit *m_axisNameEdit;
  QLabel *m_logLabel, *m_axisLabel;
};

#endif
