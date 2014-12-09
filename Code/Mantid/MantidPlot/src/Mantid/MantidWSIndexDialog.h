#ifndef MANTIDWSINDEXDIALOG_H_
#define MANTIDWSINDEXDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include <QDialog>
#include <QString>
#include <QList>
#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QValidator>
#include <QMap>

#include <set>

//----------------------------------
// Forward declarations
//----------------------------------
class MantidUI;
class IntervalList;

/** 
    The MantidWSIndexDialog class presents users with a dialog so that they may 
    specify which workspace indices / spectra IDs are to be plotted by Mantid.

    They are prompted with the available range(s) of indices/IDs they can plot.
    They must enter a range(s) that is(are) enclosed within those ranges.

    "Ranges" are of a format you've probably seen when inputting page numbers to
    print into a word processing program or similar, i.e. "2, 4-6" to print out 
    pages 2, 4, 5 and 6. 
    
    Ranges are defined by the "Interval" and "IntervalList" classes.

    The IntervalListValidator class overrides QValidator, and allows Mantid
    to assertain whether a user has attempted to input a valid range or not.
    Altering this class will affect the behaviour of what is allowed to be typed,
    and what inputs allow the "OK" button to be pressed.

    TODO - perhaps the interval objects are useful elsewhere, in which case those
           three classes are best in thier own header and source.

    @author Peter G Parker, ISIS, RAL
    @date 2011/10/06

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/

class Interval
{
public:
  /// Constructor - starting and ending at single.
  Interval(int single);
  /// Constructor - starting at start and ending at end.
  Interval(int start, int end);
  /// Constructor - attempts to parse given string to find start and end.
  Interval(QString);
  /// Copy constructor
  Interval(const Interval&);

  /// Attempts to merge the given Interval with this Interval
  bool merge(const Interval&);
  /// Returns true if it is possible to merge the given Interval with this Interval, else false.
  bool canMerge(const Interval&) const;
  /// Returns the int marking the start of this Interval
  int start() const;
  /// Returns the int marking the end of this Interval
  int end() const;
  /// Returns the length of this interval
  int length() const;

  /// Returns a set of ints that represents the interval.
  std::set<int> getIntSet() const;

  /// Returns true if this interval completely contains the interval passed to it, else false.
  bool contains(const Interval&) const;

  /// Returns a string which represents the start and end of this Interval
  std::string toStdString() const;
  /// Returns a string which represents the start and end of this Interval
  QString toQString() const;

private:
  /// Initialise the Interval, given the specified start and end ints
  void init(int, int);

  /// The start and end of the interval.
  int m_start, m_end;
};

class IntervalList
{
public:
  /// Constructor - with empty list.
  IntervalList(void);
  /// Constructor - with a list created by parsing the input string
  IntervalList(QString);
  /// Constructor - with a list containing a single Interval
  IntervalList(Interval);
  /// Copy Constructor
  IntervalList(const IntervalList&);

  /// Returns a reference to the list of Intervals.
  const QList<Interval>& getList() const;
  /// Returns the combined length of all Intervals in the list.
  int totalIntervalLength() const;

  /// Returns a string that represents the IntervalList, of the form
  /// "0, 2-5, 8, 10-12".  String is cut short by default to 6 intervals.
  std::string toStdString(int numOfIntervals = 6) const;
  /// Convenience function that returns the contents of toStdString as a QString object.
  QString toQString(int numOfIntervals = 6) const;

  /// Add an interval starting and ending at single.
  void addInterval(int single);
  /// Add an interval
  void addInterval(Interval);
  /// Add an interval starting at start and ending at end.
  void addInterval(int start, int end);
  /// Attempts to parse the given string into a IntervalList to add.
  void addIntervals(QString);
  /// Adds an IntervalList to this IntervalList.
  void addIntervalList(const IntervalList&);
  /// Replaces the current list with the list belonging to given IntervalList object.
  void setIntervalList(const IntervalList&);

  /// Returns a set of ints that represents the interval.
  std::set<int> getIntSet() const;

  /// Returns true if this interval list completely contains the interval passed to it, else false.
  bool contains(const Interval&) const;
  /// Returns true if this interval list completely contains the interval list passed to it, else false.
  bool contains(const IntervalList&) const;

  /// Returns true if the QString can be parsed into an IntervalList, else false.
  static bool isParsable(const QString&);
  /// Returns true if the QString can be parsed into an IntervalList which can then be contained
  /// in the IntervalList given, else false.
  static bool isParsable(const QString&, const IntervalList&);

  /// Returns an IntervalList which is the intersection of the given IntervalList and Interval
  static IntervalList intersect(const IntervalList&, const Interval&);
  /// Returns an IntervalList which is the intersection of the given IntervalLists
  static IntervalList intersect(const IntervalList&, const IntervalList&);

private:
  /// A list of all the Intervals in this IntervalList
  QList<Interval> m_list;
};

class IntervalListValidator : public QValidator
{
  Q_OBJECT

public:
  /// Constructor - This object must know its parent QObject, as well as the
  /// IntervalList it is validating against.
  IntervalListValidator(QObject* parent, const IntervalList& intervals);

  /// Overriden method to validate a given QString, at a particular position
  virtual State validate(QString&, int&) const;

private:
  /// The IntervalList against which to validate.
  IntervalList m_intervalList;
};
 
class MantidWSIndexDialog : public QDialog
{
  Q_OBJECT
/** Auxiliar class to wrap the QLine allowing to have a warn to the user for
 *  invalid inputs. 
*/
class QLineEditWithErrorMark : public QWidget{
public:
  /// constructor that will join togheter the QLineEdit and an 'invisible' * label.
  QLineEditWithErrorMark(QWidget * parent = 0); 
  /// virtual destructor to allow Qt to deallocate all objects
  virtual ~QLineEditWithErrorMark(){};
  /// provide acess to the QLineEdit
  QLineEdit * lineEdit(){return _lineEdit;}; 
  /// if Error is not empty, it will make the * label visible and set the tooltip as the error.
  void setError(QString error); 
private: 
  QLineEdit * _lineEdit; 
  QLabel * m_validLbl;
};

public:
  /// Constructor - same parameters as one of the parent constructors, along with a 
  /// list of the names of workspaces to be plotted.
  MantidWSIndexDialog(MantidUI* parent, Qt::WFlags flags, QList<QString> wsNames);

  /// Returns the QMultiMap that contains all the workspaces that are to be plotted, 
  /// mapped to the set of workspace indices.
  QMultiMap<QString,std::set<int> > getPlots() const;

private slots:
  /// Called when the OK button is pressed.
  void plot();
  /// Called when the "Plot All" button is pressed.
  void plotAll();
  /// Called when the wsField has been edited.
  void editedWsField();
  /// Called when the spectraField has been edited.
  void editedSpectraField();

private:
  /// Initializes the layout of the dialog
  void init();
  /// Initializes the layout of the workspace index section of the dialog
  void initWorkspaceBox();
  /// Initializes the layout of the spectra ID section of the dialog
  void initSpectraBox();
  /// Initializes the layout of the buttons
  void initButtons();

  /// Check to see if all workspaces have a spectrum axis
  void checkForSpectraAxes();

  /// Generates an IntervalList which defines which workspace indices the user can
  /// ask to plot.
  void generateWsIndexIntervals();
  /// Generates an IntervalList which defines which spectra IDs the user can ask to plot.
  void generateSpectraIdIntervals();

  /// Whether or not there are any common spectra IDs between workspaces.
  bool usingSpectraIDs() const;

  /// A pointer to the parent MantidUI object
  MantidUI* m_mantidUI;

  /// Do we allow the user to ask for a range of spectra IDs or not?
  bool m_spectra;

  /// Pointers to the obligatory Qt objects:
  QLabel *m_wsMessage, *m_spectraMessage, *m_orMessage;
  QLineEditWithErrorMark *m_wsField, *m_spectraField;
  QVBoxLayout *m_outer, *m_wsBox, *m_spectraBox; 
  QHBoxLayout *m_buttonBox;
  QPushButton *m_okButton, *m_cancelButton, *m_plotAllButton;

  /// A list of names of workspaces which are to be plotted.
  QList<QString> m_wsNames;
  /// IntervalLists for the range of indices/IDs AVAILABLE to the user.
  IntervalList m_wsIndexIntervals, m_spectraIdIntervals;
  /// IntervalLists for the range of indices/IDs CHOSEN by the user.
  IntervalList m_wsIndexChoice, m_spectraIdChoice;
};

#endif //MANTIDWSINDEXDIALOG_H_

