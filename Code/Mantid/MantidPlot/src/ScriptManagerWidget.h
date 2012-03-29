#ifndef SCRIPTMANAGERWIDGET_H_
#define SCRIPTMANAGERWIDGET_H_

//---------------------------------------------------------
// Includes
//---------------------------------------------------------
#include <QTabWidget>
#include <QDialog>
#include "Scripted.h"

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

  ///Save settings applicable to the manager
  void saveSettings();

  /// Current interpreter
  ScriptFileInterpreter * currentInterpreter();
  /// Interpreter at given index
  ScriptFileInterpreter * interpreterAt(int index);

  // ----------------------- Menus --------------------------------------------
  /// Add the required entries for the file menu in this context
  void populateFileMenu(QMenu &fileMenu);
  /// Add the required entries for the edit menu in this context
  void populateEditMenu(QMenu &editMenu);
  /// Add the required entries for the execute menu in this context
  void populateExecMenu(QMenu &execMenu);
  /// Add the required entries for a Window menu in this context
  void populateWindowMenu(QMenu &windowMenu);


  /// Is a script running in the environment
  bool isScriptRunning();

 /// this method appends the file names of scripts
 ///in different tabs to a string and returns 
  QString saveToString();
  ///this method returns a list containing  recent scripts
  QStringList recentScripts();
  /// update the Recent Scripts menu items
  void updateRecentScriptList();
  ///set the recent script list
  void setRecentScripts(const QStringList & scriptList);
  /// copy implementation
  void copy();
  /// paste implementation
  void paste();

public slots:
  /// Create a new tab for script editing with the text within the file imported and insert it at the index
  void newTab(int index = -1, const QString & filename = "");
  /// Open a file in the current tab
  void openInCurrentTab(const QString & filename = QString());
  /// Open a file in a new tab
  void openInNewTab(const QString & filename = QString());
  /// Close all tabs
  void closeAllTabs();
  /// Show the find dialog
  void showFindDialog(bool replace = true);

  /** @name Execute members.*/
  //@{
  /// Execute
  void executeAll();
  ///Execute all
  void executeSelection();
  /// Evaluate
  void evaluate();

private slots:
  /// Context menu handler
  void editorContextMenu(const QPoint & pos);
  /// Close current tab
  int closeCurrentTab();
  /// Close clicked tab
  void closeClickedTab();
  /// Mark the current tab
  void markCurrentAsChanged();
  /// Current tab has changed
  void tabSelectionChanged(int index);
  /// Enable/disable the relevant actions based on the execution state of the script
  void setScriptIsRunning(bool running);
  /// Toggle the progress reporting arrow
  void toggleProgressArrow(bool on);
  /// Toggle code folding
  void toggleCodeFolding(bool on);
  /// Toggle code folding
  void toggleCodeCompletion(bool on);
  /// Toggle call tips
  void toggleCallTips(bool on);
  /// open recent scripts
  void openRecentScript(int index);

private:
  /// Initialize the actions relevant to this object
  void initActions();
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

//  ///Set auto complete behaviour for the given editor
//  void setCodeCompletionBehaviour(ScriptEditor *editor, bool state);
//  ///Set auto complete behaviour for the given editor
//  void setCallTipsBehaviour(ScriptEditor *editor, bool state);
//  ///Set auto complete behaviour for the given editor
//  void setCodeFoldingBehaviour(ScriptEditor *editor, bool state);

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

  /// File actions
  QAction *m_new_tab, *m_open_curtab, *m_open_newtab, *m_save, *m_saveas, *m_close_tab;

  ///Edit actions that are necessary for the manager
  QAction *m_find;

  ///Toggle progress
  QAction *m_toggle_progress;
  /// Toggle code folding
  QAction *m_toggle_folding;
  /// Toggle code folding
  QAction *m_toggle_completion;
  /// Toggle code folding
  QAction *m_toggle_calltips;

  /// Recent scripts option
  QMenu* m_recent_scripts;

  ///Display mode boolean
  bool m_interpreter_mode;
  ///list storing the recent scripts
  QStringList m_recentScriptList;
  /// Flag to indicate whether stdout should be redirected
  bool m_capturePrint;
  /// enum used for maximum of recent scripts size
  enum {MaxRecentScripts = 5};
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
