#ifndef SCRIPTMANAGERWIDGET_H_
#define SCRIPTMANAGERWIDGET_H_

//---------------------------------------------------------
// Includes
//---------------------------------------------------------
#include <QTabWidget>
#include <QDialog>
#include <QHash>
#include "Scripted.h"

//---------------------------------------------------------
// Forward declarations
//--------------------------------------------------------
class QTabWidget;
class QPoint;
class ScriptingWindow;
class ScriptOutputDock;
class ScriptEditor;
class Script;
class QAction;
class QPushButton;
class QCheckBox;
class QComboBox;

class FindReplaceDialog;



/** 
    This class manages ScriptEdit objects and displays them in a series
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
  /// Qt macro so that we can use the signal/slot 
  Q_OBJECT

public:
  /// Constructor
  ScriptManagerWidget(ScriptingEnv *env, QWidget *parent, bool interpreter_mode = false);
  ///Destructor
  ~ScriptManagerWidget();
  ///Save settings applicable to the manager
  void saveSettings();  
  /// Ask if we should save
  void askSave(int index);
  /// Open a script from a file and read the file into a QString. 
  /// QStrings are implicity shared so the return, seemingly by value, is not expensive
  QString readScript(const QString& filename, bool *ok);
  /// Is a script running in the environment
  bool isScriptRunning();
  /// Return the current editor
  ScriptEditor *currentEditor() const;
  /// Undo action for the current editor
  QAction* undoAction() const;
  /// Redo action for the current editor
  QAction* redoAction() const;
  /// Cut action for the current editor
  QAction* cutAction() const;
  /// Copy action for the current editor
  QAction* copyAction() const;
  /// Paste action for the current editor
  QAction* pasteAction() const;
  /// Print action for the current editor
  QAction* printAction() const;
 /// this method appens the file names of scripts
 ///in different tabs to a string and returns 
  QString saveToString();


signals:
  ///A message is ready to be printed
  void MessageToPrint(const QString & msg, bool error, bool timestamp = false);    
  ///A script has changed execution state
  void ScriptIsActive(bool running);

public slots:
  /// Create a new tab for script editing with the text within the file imported
  ScriptEditor* newTab(int index = -1);
  /// Open a file in the current tab
  void openInCurrentTab(const QString & filename = QString());
  /// Open a file in a new tab
  void openInNewTab(const QString & filename = QString());
  ///Save file to different file name
  QString saveAs(int index = -1);
  /// Save file
  void save(int index = -1);
  /// Close all tabs
  void closeAllTabs();
  /// Show the find dialog
  void showFindDialog(bool replace = true);

  /// comiple the code
  void compile(const QString &);

  /** @name Execute members.*/
  //@{
  /// Execute
  void execute();
  ///Execute all
  void executeAll();
  /// Evaluate
  void evaluate();
  ///Execute an interpreter line
  void executeInterpreter(const QString & code);
  //@}
  ///Run script code
  bool runScriptCode(const QString & code, const int line_offset = 0);

  /// run mutli line code
  bool runMultiLineCode();

  ///Format an output message with an optional timestamp
  void displayOutput(const QString & msg, bool timestamp = false);
  ///Format an output message with an optional timestamp
  void displayError(const QString & msg, bool timestamp = false);
					
private slots:
  /// Context menu handler
  void editorContextMenu(const QPoint & pos);
  /// Close current tab
  int closeCurrentTab();
  /// Close clicked tab
  void closeClickedTab();
  /// Mark as changed
  void markCurrentAsChanged();
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
  ///execute multi line code
  void executeMultiLine();

private:
  /// Initialize the actions relevant to this object
  void initActions();
  /// A context menu event for the tab widget itself
  void contextMenuEvent(QContextMenuEvent *event);
  /// A custom defined event handler
  void customEvent(QEvent *event);
  ///Open a script
  void open(bool newtab, const QString & filename = QString());
  /// Create a new Script object and connect up the relevant signals.
  Script * createScriptRunner(ScriptEditor *editor);
  ///Close a tab with a given index
  void closeTabAtIndex(int index);
  ///Close a tab at a given position
  void closeTabAtPosition(const QPoint & pos);
  ///Does the actual saving of a file
  void doSave(ScriptEditor * editor);
  ///Set auto complete behaviour for the given editor
  void setCodeCompletionBehaviour(ScriptEditor *editor, bool state);
  ///Set auto complete behaviour for the given editor
  void setCallTipsBehaviour(ScriptEditor *editor, bool state);
  ///Set auto complete behaviour for the given editor
  void setCodeFoldingBehaviour(ScriptEditor *editor, bool state);

 private:
  /// So that the window can access the actions that are relevant
  friend class ScriptingWindow;
  friend class ScriptOutputDock;

  /// The last directory visted with a file dialog
  QString m_last_dir;
  /// The script objects for each tab that will execute the code
  QHash<int, Script *> m_script_runners;
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
  /// Script execute actions
  QAction *m_exec, *m_exec_all, *m_eval;
  ///Toggle progress
  QAction *m_toggle_progress;
  /// Toggle code folding
  QAction *m_toggle_folding;
  /// Toggle code folding
  QAction *m_toggle_completion;
  /// Toggle code folding
  QAction *m_toggle_calltips;

  /// The find replace dialog
  FindReplaceDialog *m_findrep_dlg;

  ///Display mode boolean
  bool m_interpreter_mode;
};


/**
   @class FindReplaceDialog

   This class raises a dialog to find and optionally replace text within in a text edit. 
   Note: It came from the main qtiplot repository at r1341 and it has been modified to
   work with our script window (also added comments). Since it's keyed to work only with Script editing
   classes, it's definition may aswell just go here
 */
class FindReplaceDialog : public QDialog
{
  // Qt macro
  Q_OBJECT

public:
  ///Constructor
  FindReplaceDialog(ScriptManagerWidget *manager, bool replace = false, 
		    QWidget* parent = 0, Qt::WindowFlags fl = 0 );

public slots:
  /// An option has been toggled or the manager has some update that is necessary
  void resetSearchFlag();

protected slots:
  /// Find 
  bool find(bool backwards = false);
  /// Replace slot
  void replace();
  /// Replace all slot
  void replaceAll();

private slots:
  /// A slot for the findClicked button
  void findClicked();

private:
  ///The current text editor we are working on
  ScriptManagerWidget *m_manager;
  
  ///Find next match button
  QPushButton* buttonNext;
  /// Replace text button
  QPushButton* buttonReplace;
  /// Replace all text button
  QPushButton* buttonReplaceAll;
  /// Cancel dialog button
  QPushButton* buttonCancel;
  
  /// Find box
  QComboBox* boxFind;
  /// Replace box
  QComboBox* boxReplace;
  
  /// Case-sensitive check box
  QCheckBox *boxCaseSensitive;
  /// Whole words check box
  QCheckBox *boxWholeWords;
  /// Search backwards
  QCheckBox *boxSearchBackwards;
  /// Wrap around
  QCheckBox *boxWrapAround;
  /// Treat as regular expressions
  QCheckBox *boxRegex;

  /// If a find is in progress
  bool m_find_inprogress;
};


#endif
