#ifndef SCRIPTMANAGERWIDGET_H_
#define SCRIPTMANAGERWIDGET_H_

//---------------------------------------------------------
// Includes
//---------------------------------------------------------
#include <QTabWidget>
#include <QDialog>
#include "Scripted.h"
#include "Script.h"

//---------------------------------------------------------
// Forward declarations
//--------------------------------------------------------
class QTabWidget;
class QPoint;
class QAction;
class QPushButton;
class QCheckBox;
class QComboBox;
class ScriptFileInterpreter;
class NullScriptFileInterpreter;

class FindReplaceDialog;
class QMenu;
class QStringList;

class ScriptingWindow;


/** 
    This class manages ScriptEditor objects and displays them in a series
    of tabs. It is also the single point of entry for executing scripts
    with in the current ScriptingEnv
    
    @author Martyn Gigg, Tessella Support Services plc
    @date 19/08/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class ScriptManagerWidget : public QTabWidget, Scripted
{
  Q_OBJECT

public:
  /// Constructor
  ScriptManagerWidget(ScriptingEnv *env, QWidget *parent);
  ///Destructor
  ~ScriptManagerWidget();

  /// Current interpreter
  ScriptFileInterpreter * currentInterpreter();
  /// Interpreter at given index
  ScriptFileInterpreter * interpreterAt(int index);

  /// Is a script running in the environment
  bool isExecuting();

 /// this method appends the file names of scripts
 ///in different tabs to a string and returns 
  QString saveToString();
  ///this method returns a list containing  recent scripts
  QStringList recentScripts();
  /// update the Recent Scripts menu items
  void updateRecentScriptList(const QString & filename);
  ///set the recent script list
  void setRecentScripts(const QStringList & scriptList);

public slots:
  /// Create a new tab for script editing with the text within the file imported and insert it at the index
  void newTab(int index = -1, const QString & filename = "");
  /// Open a file in the current tab
  void openInCurrentTab(const QString & filename = QString());
  /// Open a file in a new tab
  void openInNewTab(const QString & filename = QString());
  /// open recent scripts
  void openRecentScript(int index);
  /// Save current file
  void saveToCurrentFile();
  /// Save to new file
  void saveAs();
  /// Print the current script
  void print();
  /// Close current tab
  int closeCurrentTab();
  /// Close all tabs
  void closeAllTabs();
  /// Show the find dialog
  void showFindDialog(bool replace = true);

  /// undo
  void undo();
  /// redo
  void redo();
  /// cut current
  void cut();
  /// copy implementation
  void copy();
  /// paste implementation
  void paste();
  /// find
  void findInScript();

  /** @name Execute members.*/
  //@{
  /// Execute
  void executeAll(const Script::ExecutionMode mode = Script::Asynchronous);
  ///Execute all
  void executeSelection(const Script::ExecutionMode mode = Script::Asynchronous);
  /// Evaluate
  void evaluate();

  /// Increase font size
  void zoomIn();
  /// Decrease font size
  void zoomOut();

  /// Toggle the progress reporting arrow
  void toggleProgressArrow(bool on);
  /// Toggle code folding
  void toggleCodeFolding(bool on);
  /// Toggle code folding
  void toggleCodeCompletion(bool on);
  /// Toggle call tips
  void toggleCallTips(bool on);

private slots:
  /// Close clicked tab
  void closeClickedTab();
  /// Mark the current tab
  void markCurrentAsChanged();
  /// Current tab has changed
  void tabSelectionChanged(int index);
  /// Enable/disable the relevant actions based on the execution state of the script
  void setScriptIsRunning(bool running);

private:
  /// A context menu event for the tab widget itself
  void contextMenuEvent(QContextMenuEvent *event);
  /// A custom defined event handler
  void customEvent(QEvent *event);
  ///Open a script
  void open(bool newtab, const QString & filename = QString());
  ///Close a tab with a given index
  void closeTabAtIndex(int index);
  ///Close a tab at a given position
  void closeTabAtPosition(const QPoint & pos);

 private:
  friend class ScriptingWindow;

  /// The last directory visited with a file dialog
  QString m_last_dir;
  // The cursor position within the tab bar when the right-mouse button was last clicked
  // I need this to ensure that the position of a call to tabBar()->tabAt() is accurate
  // as Qt doesn't provide an action signal parameterized on a position
  QPoint m_cursor_pos;
  /// The index of the last active tab 
  int m_last_active_tab;

  /// enum used for maximum of recent scripts size
  enum { MaxRecentScripts = 5 };
  /// List of recent scripts, with most recent at the top
  QStringList m_recentScriptList;
  /// Flag to indicate whether stdout should be redirected
  bool m_capturePrint;
  /// A pointer to the Null object
  NullScriptFileInterpreter *m_nullScript;
  /// A pointer to the current interpreter
  ScriptFileInterpreter *m_current;
};


/**
   @class FindReplaceDialog

   This class raises a dialog to find and optionally replace text within in a text edit. 
   Note: It came from the main qtiplot repository at r1341 and it has been modified to
   work with our script window (also added comments). Since it's keyed to work only with Script editing
   classes, it's definition may as well just go here
 */
//class FindReplaceDialog : public QDialog
//{
//  // Qt macro
//  Q_OBJECT
//
//public:
//  ///Constructor
//  FindReplaceDialog(ScriptManagerWidget *manager, bool replace = false,
//		    QWidget* parent = 0, Qt::WindowFlags fl = 0 );
//
//public slots:
//  /// An option has been toggled or the manager has some update that is necessary
//  void resetSearchFlag();
//
//protected slots:
//  /// Find
//  bool find(bool backwards = false);
//  /// Replace slot
//  void replace();
//  /// Replace all slot
//  void replaceAll();
//
//private slots:
//  /// A slot for the findClicked button
//  void findClicked();
//
//private:
//  ///The current text editor we are working on
//  ScriptManagerWidget *m_manager;
//
//  ///Find next match button
//  QPushButton* buttonNext;
//  /// Replace text button
//  QPushButton* buttonReplace;
//  /// Replace all text button
//  QPushButton* buttonReplaceAll;
//  /// Cancel dialog button
//  QPushButton* buttonCancel;
//
//  /// Find box
//  QComboBox* boxFind;
//  /// Replace box
//  QComboBox* boxReplace;
//
//  /// Case-sensitive check box
//  QCheckBox *boxCaseSensitive;
//  /// Whole words check box
//  QCheckBox *boxWholeWords;
//  /// Search backwards
//  QCheckBox *boxSearchBackwards;
//  /// Wrap around
//  QCheckBox *boxWrapAround;
//  /// Treat as regular expressions
//  QCheckBox *boxRegex;
//
//  /// If a find is in progress
//  bool m_find_inprogress;
//};


#endif
