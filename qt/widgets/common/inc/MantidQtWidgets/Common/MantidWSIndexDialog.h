// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------
// Includes
//----------------------------------
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QString>
#include <QVBoxLayout>
#include <QValidator>

#include <optional>
#include <set>

namespace MantidQt {
namespace MantidWidgets {
//----------------------------------
// Forward declarations
//----------------------------------
class IntervalList;

/**
        The MantidWSIndexDialog class presents users with a dialog so that
   they may specify which workspace indices / spectra IDs are to be plotted
   by Mantid and the manner by which they are plotted.

        They are prompted with the available range(s) of indices/IDs they
   can plot.They must enter a range(s) that is(are) enclosed within those
   ranges.

        "Ranges" are of a format you've probably seen when inputting page
   numbers to print into a word processing program or similar,
   i.e. "2, 4-6" to print out pages 2, 4, 5 and 6.

        Ranges are defined by the "Interval" and "IntervalList" classes.

        The IntervalListValidator class overrides QValidator, and allows
   Mantid to assertain whether a user has attempted to input a valid range
   or not. Altering this class will affect the behaviour of what is allowed
   to be typed, and what inputs allow the "OK" button to be pressed.

        TODO - perhaps the interval objects are useful elsewhere, in which
   case those three classes are best in their own header and source.

        This dialog also enables one to choose how to do the plotting.
   One can choose between simple 1D plot, waterfall or tiled plot.
   Also the advanced form of this dialog has surface and contour plot
   and enables you to put a log value into the plot instead of the
   worksapce index or spectrum number.

        @author Peter G Parker, ISIS, RAL
        @date 2011/10/06
*/

class EXPORT_OPT_MANTIDQT_COMMON Interval {
public:
  /// Constructor - starting and ending at single.
  explicit Interval(int single);
  /// Constructor - starting at start and ending at end.
  Interval(int start, int end);
  /// Constructor - attempts to parse given string to find start and end.
  explicit Interval(const QString & /*intervalString*/);
  /// Copy constructor
  Interval(const Interval & /*copy*/);

  /// Attempts to merge the given Interval with this Interval
  bool merge(const Interval & /*other*/);
  /// Returns true if it is possible to merge the given Interval with this
  /// Interval, else false.
  bool canMerge(const Interval & /*other*/) const;
  /// Returns the int marking the start of this Interval
  int start() const;
  /// Returns the int marking the end of this Interval
  int end() const;
  /// Returns the length of this interval
  int length() const;

  /// Returns a set of ints that represents the interval.
  std::set<int> getIntSet() const;

  /// Returns true if this interval completely contains the interval passed
  /// to
  /// it, else false.
  bool contains(const Interval & /*other*/) const;

  /// Returns a string which represents the start and end of this Interval
  std::string toStdString() const;
  /// Returns a string which represents the start and end of this Interval
  QString toQString() const;

private:
  /// Initialise the Interval, given the specified start and end ints
  void init(int /*start*/, int /*end*/);

  /// The start and end of the interval.
  int m_start, m_end;
};

class EXPORT_OPT_MANTIDQT_COMMON IntervalList {
public:
  /// Constructor - with empty list.
  IntervalList(void);
  /// Constructor - with a list created by parsing the input string
  explicit IntervalList(const QString & /*intervals*/);
  /// Constructor - with a list containing a single Interval
  explicit IntervalList(const Interval & /*interval*/);

  /// Returns a reference to the list of Intervals.
  const QList<Interval> &getList() const;
  /// Returns the combined length of all Intervals in the list.
  int totalIntervalLength() const;

  /// Returns a string that represents the IntervalList, of the form
  /// "0, 2-5, 8, 10-12".  String is cut short by default to 6 intervals.
  std::string toStdString(int numOfIntervals = 6) const;
  /// Convenience function that returns the contents of toStdString as a
  /// QString
  /// object.
  QString toQString(int numOfIntervals = 6) const;

  /// Add an interval starting and ending at single.
  void addInterval(int single);
  /// Add an interval
  void addInterval(Interval /*interval*/);
  /// Add an interval starting at start and ending at end.
  void addInterval(int start, int end);
  /// Attempts to parse the given string into a IntervalList to add.
  void addIntervals(QString /*intervals*/);
  /// Adds an IntervalList to this IntervalList.
  void addIntervalList(const IntervalList & /*intervals*/);
  /// Replaces the current list with the list belonging to given
  /// IntervalList
  /// object.
  void setIntervalList(const IntervalList & /*intervals*/);
  /// Clears the interval list
  void clear();

  /// Returns a set of ints that represents the interval.
  std::set<int> getIntSet() const;

  /// Returns true if this interval list completely contains the interval
  /// passed
  /// to it, else false.
  bool contains(const Interval & /*other*/) const;
  /// Returns true if this interval list completely contains the interval
  /// list
  /// passed to it, else false.
  bool contains(const IntervalList & /*other*/) const;

  /// Returns true if the QString can be parsed into an IntervalList, else
  /// false.
  static bool isParsable(const QString & /*input*/);
  /// Returns true if the QString can be parsed into an IntervalList which
  /// can
  /// then be contained
  /// in the IntervalList given, else false.
  static bool isParsable(const QString & /*input*/, const IntervalList & /*container*/);

  /// Returns an IntervalList which is the intersection of the given
  /// IntervalList and Interval
  static IntervalList intersect(const IntervalList & /*aList*/, const Interval & /*bInterval*/);
  /// Returns an IntervalList which is the intersection of the given
  /// IntervalLists
  static IntervalList intersect(const IntervalList & /*a*/, const IntervalList & /*b*/);

private:
  /// A list of all the Intervals in this IntervalList
  QList<Interval> m_list;
};

class EXPORT_OPT_MANTIDQT_COMMON IntervalListValidator : public QValidator {
  Q_OBJECT

public:
  /// Constructor - This object must know its parent QObject, as well as the
  /// IntervalList it is validating against.
  IntervalListValidator(QObject *parent, IntervalList intervals);

  /// Overriden method to validate a given QString, at a particular position
  State validate(QString & /*unused*/, int & /*unused*/) const override;

private:
  /// The IntervalList against which to validate.
  IntervalList m_intervalList;
};

class EXPORT_OPT_MANTIDQT_COMMON MantidWSIndexWidget : public QWidget {
  Q_OBJECT

  /** Auxiliary class to wrap the QLineEdit allowing warning to the user
   * for invalid inputs.
   */
  class QLineEditWithErrorMark : public QWidget {
  public:
    /// constructor to join together the QLineEdit and an 'invisible'
    /// *
    /// label.
    explicit QLineEditWithErrorMark(QWidget *parent = nullptr);
    /// virtual destructor to allow Qt to deallocate all objects
    ~QLineEditWithErrorMark() override {};
    /// provide acess to the QLineEdit
    QLineEdit *lineEdit() { return _lineEdit; };
    /// if Error is not empty, it will make the * label visible and set the
    /// tooltip as the error.
    void setError(const QString &error);

  private:
    QLineEdit *_lineEdit;
    QLabel *m_validLbl;
  };

public:
  /**
   * Plain old data structures to hold all user-selected input
   */

  /// Struct to hold user input
  struct UserInputAdvanced {
    bool accepted;
    int plotIndex;
    QString axisName;
    QString logName;
    std::set<double> customLogValues;
    QList<QString> workspaceNames;
  };

  struct UserInput {
    QMultiMap<QString, std::set<int>> plots;
    bool simple;
    bool waterfall;
    bool tiled;
    bool errors;
    bool surface;
    bool contour;
    bool isAdvanced;
    std::optional<UserInputAdvanced> advanced;
  };

  /// The string "Workspace index"
  static const QString WORKSPACE_NAME;
  static const QString WORKSPACE_INDEX;
  /// The string "Custom"
  static const QString CUSTOM;
  /// Strings for plot types
  static const QString SIMPLE_PLOT;
  static const QString WATERFALL_PLOT;
  static const QString SURFACE_PLOT;
  static const QString CONTOUR_PLOT;

  /// Constructor - same parameters as one of the parent constructors, along
  /// with a
  /// list of the names of workspaces to be plotted.
  MantidWSIndexWidget(QWidget *parent, const Qt::WindowFlags &flags, const QList<QString> &wsNames,
                      const bool showWaterfallOption = false, const bool showTiledOption = false,
                      const bool isAdvanced = false);

  /// Returns a structure holding all of the selected options
  UserInput getSelections();
  /// Returns the QMultiMap that contains all the workspaces that are to be
  /// plotted,
  /// mapped to the set of workspace indices.
  QMultiMap<QString, std::set<int>> getPlots() const;
  /// Returns whether the simple 1D plot option has been selected
  bool is1DPlotSelected() const;
  /// Returns whether the waterfall option has been selected
  bool isWaterfallPlotSelected() const;
  /// Called by dialog when plot requested
  bool plotRequested();
  /// Called by dialog when plot all requested
  bool plotAllRequested();
  /// Validate plot options when either plot or plot all is requested
  bool validatePlotOptions();
  /// Returns whether the tiled plot option has been selected
  bool isTiledPlotSelected() const;
  /// Returns whether surface plot is selected
  bool isSurfacePlotSelected() const;
  /// Returns whether contour plot is selected
  bool isContourPlotSelected() const;
  /// Returns whether the error bars option has been selected
  bool isErrorBarsSelected() const;

private slots:
  /// Called when the wsField has been edited.
  void editedWsField();
  /// Called when the spectraField has been edited.
  void editedSpectraField();
  /// Called when the log selection is changed.
  void onLogSelected(const QString &logName);
  /// Called when the plot option has changed.
  void onPlotOptionChanged(const QString &logName);

private:
  /// Initializes the layout of the dialog
  void init();
  /// Initializes the layout of the workspace index section of the dialog
  void initWorkspaceBox();
  /// Initializes the layout of the spectra ID section of the dialog
  void initSpectraBox();
  /// Initialize the layout of the options check boxes
  void initOptionsBoxes();
  /// Initializes the layout of the log options
  void initLogs();
  /// Populate the log combo box
  void populateLogComboBox();
  /// Get a handle a workspace by name
  Mantid::API::MatrixWorkspace_const_sptr getWorkspace(const QString &workspaceName) const;
  /// Check if workspaces are suitable for contour or surface plot
  bool isSuitableForContourOrSurfacePlot() const;
  /// Check if workspaces are suitable for use of log values
  bool isSuitableForLogValues(const QString &plotOption) const;
  /// Gets the axis name
  const QString getAxisName() const;
  /// Gets the log name
  const QString getLogName() const;
  /// Get the set of custom log values
  const std::set<double> getCustomLogValues() const;
  /// Provide warning if there are plot errors.
  void showPlotOptionsError(const QString &message);
  /// Get the plot index
  int getPlotIndex() const;

  /// Check to see if all workspaces have a spectrum axis
  void checkForSpectraAxes();

  /// Generates an IntervalList which defines which workspace indices the
  /// user can ask to plot.
  void generateWsIndexIntervals();
  /// Generates an IntervalList which defines which spectra IDs the
  /// user can ask to plot.
  void generateSpectraNumIntervals();

  /// Whether or not there are any common spectra IDs between workspaces.
  bool usingSpectraNumbers() const;

  /// Do we allow the user to ask for a range of spectra IDs or not?
  bool m_spectra;

  /// Do we allow the display of the waterfall option
  bool m_waterfall;

  /// Do we allow the display of the tiled option
  bool m_tiled;

  /// Is the plotting advanced?
  bool m_advanced;

  /// Pointers to the obligatory Qt objects:
  QLabel *m_wsMessage, *m_spectraMessage, *m_orMessage, *m_plotOptionLabel, *m_logLabel, *m_customLogLabel,
      *m_axisLabel;
  QLineEditWithErrorMark *m_wsField, *m_spectraField, *m_logValues;
  QGroupBox *m_logOptionsGroup;
  QVBoxLayout *m_outer, *m_wsBox, *m_spectraBox, *m_logBox, *m_optionsBox;
  QComboBox *m_plotOptions, *m_logSelector;
  QCheckBox *m_showErrorBars;
  QLineEditWithErrorMark *m_axisNameEdit;

  /// A list of names of workspaces which are to be plotted.
  QList<QString> m_wsNames;
  /// IntervalLists for the range of indices/numbers AVAILABLE to the user.
  IntervalList m_wsIndexIntervals, m_spectraNumIntervals;
  /// IntervalLists for the range of indices/numbers CHOSEN by the user.
  IntervalList m_wsIndexChoice, m_spectraNumChoice;
  /// Flags to indicate which one of the two interval lists above is chosen by
  /// user
  bool m_usingWsIndexChoice = false, m_usingSprectraNumChoice = false;
};

class EXPORT_OPT_MANTIDQT_COMMON MantidWSIndexDialog : public QDialog {
  Q_OBJECT

public:
  /// Constructor - has a list of the names of workspaces to be plotted.
  MantidWSIndexDialog(QWidget *parent, const Qt::WindowFlags &flags, const QList<QString> &wsNames,
                      const bool showWaterfallOption = false, const bool showPlotAll = true,
                      const bool showTiledOption = false, const bool isAdvanced = false);
  /// Returns a structure holding all of the selected options
  MantidWSIndexWidget::UserInput getSelections();
  /// Returns the QMultiMap that contains all the workspaces that are to be
  /// plotted,
  /// mapped to the set of workspace indices.
  QMultiMap<QString, std::set<int>> getPlots() const;
  /// Returns whether the waterfall option has been selected
  bool is1DPlotSelected() const;
  /// Returns whether the waterfall option has been selected
  bool isWaterfallPlotSelected() const;
  /// Returns whether the tiled plot option has been selected
  bool isTiledPlotSelected() const;
  /// Returns whether surface plot is selected
  bool isSurfacePlotSelected() const;
  /// Returns whether surface plot is selected
  bool isContourPlotSelected() const;
  /// Returns whether error bars have been selected
  bool isErrorBarsSelected() const;
private slots:
  /// Called when the OK button is pressed.
  void plot();
  /// Called when the "Plot All" button is pressed.
  void plotAll();

private:
  MantidWSIndexWidget m_widget;
  /// Initializes the layout of the dialog
  void init(bool isAdvanced);
  /// Initializes the layout of the buttons
  void initButtons();
  /// Do we allow the display of the "Plot all" button
  bool m_plotAll;
  /// Qt objects
  QPushButton *m_okButton, *m_cancelButton, *m_plotAllButton;
  QHBoxLayout *m_buttonBox;
  QVBoxLayout *m_outer;
};
} // namespace MantidWidgets
} // namespace MantidQt
