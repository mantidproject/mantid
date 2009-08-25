#ifndef SCRIPTMANAGERWIDGET_H_
#define SCRIPTMANAGERWIDGET_H_

//----------------------------------
// Includes
//----------------------------------
#include <QTabWidget>
#include "Script.h"

//----------------------------------
// Forward declarations
//----------------------------------
class QTabWidget;
class QPoint;
class ScriptEditor;
class QAction;
class ScriptingWindow;

/** 
    This class manages ScriptEdit objects and displays them in a series
    of tabs. It is also the single point of entry for executing scripts
    with in the current ScriptingEnv
    
    @author Martyn Gigg, Tessella Support Services plc
    @date 19/08/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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
class ScriptManagerWidget : public QTabWidget, scripted
{
  /// Qt macro so that we can use the signal/slot 
  Q_OBJECT

public:
  /// Constructor
  ScriptManagerWidget(ScriptingEnv *env, QWidget *parent);
  ///Destructor
  ~ScriptManagerWidget();
  /// Ask if we should save
  void askSave(int index);
  /// Open a script from a file and read the file into a QString. 
  /// QStrings are implicity shared so the return, seemingly by value, is not expensive
  QString readScript(const QString& filename);
  ///Run script code
  bool runScriptCode(const QString & code);
  /// Is a script running?
  inline bool isScriptRunning() const
  {
    return m_script_executing;
  }

signals:
  ///A message is ready to be printed
  void MessageToPrint(const QString & msg, bool error);    
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
  
  /** @name Execute members.*/
  //@{
  /// Execute
  void execute();
  ///Execute all
  void executeAll();
  /// Evaluate
  void evaluate();
  //@}

  ///Format an output message
  void formatOutput(const QString & msg);
  ///Format an output message
  void formatError(const QString & msg);
					
private slots:
  /// Context menu handler
  void editorContextMenu(const QPoint & pos);
  /// Close current tab
  int closeCurrentTab();
  /// Close clicked tab
  void closeClickedTab();
  /// Update for tab switch
  void newTabSelected();
  /// Mark as changed
  void markCurrentAsChanged();
  /// Enable/disable the relevant actions based on the execution state of the script
  void setScriptIsRunning(bool running);

private:
  /// Initialize the actions relevant to this object
  void initActions();
  /// A context menu event for the tab widget itself
  void contextMenuEvent(QContextMenuEvent *event);
  /// A custom defined event handler
  void customEvent(QEvent *event);
  ///Open a script
  void open(bool newtab, const QString & filename = QString());
  /// Format a message and emit the formatted string to be printed
  void formatMessage(const QString & msg, bool error);
  /// Create a new Script object and connect up the relevant signals.
  void setNewScriptRunner();
  ///Close a tab with a given index
  void closeTabAtIndex(int index);
  ///Close a tab at a given position
  void closeTabAtPosition(const QPoint & pos);  

 private:
  /// So that the window can access the actions that are relevant
  friend class ScriptingWindow;

  /// The last directory visted with a file dialog
  QString m_last_dir;
  /// The script object that will execute the code
  Script *m_script_runner;
  // A flag to store if a script is being executed
  bool m_script_executing;
  // The cursor position within the tab bar when the right-mouse button was last clicked
  // I need this to ensure that the position of a call to tabBar()->tabAt() is accurate
  // as Qt doesn't provide an action signal parameterized on a position
  QPoint m_cursor_pos;

  /// File actions
  QAction *m_new_tab, *m_open_curtab, *m_open_newtab, *m_save, *m_saveas, *m_close_tab;
  ///Edit actions
  QAction *m_undo, *m_redo, *m_cut, *m_copy, *m_paste;
  /// Script execute actions
  QAction *m_exec, *m_exec_all, *m_eval;
};

#endif
