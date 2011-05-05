#ifndef SCRIPTINGWINDOW_H_
#define SCRIPTINGWINDOW_H_

//----------------------------------
// Includes
//----------------------------------
#include <QMainWindow>
#include <QDockWidget>

//----------------------------------------------------------
// Forward declarations
//---------------------------------------------------------
class ScriptManagerWidget;
class ScriptingEnv;
class QTextEdit;
class QPoint;
class QMenu;
class QAction;
class QCloseEvent;
class QShowEvent;
class QHideEvent;

/** @class ScriptOutputDock
    
   This class holds any output from executed scripts. It defines a custom context menu
   that allows the text to be cleared, copied and printed. Note: Ideally this would be
   nested inside ScriptWindow as it is not needed anywhere else, but the Qt SIGNAL/SLOTS
   mechanism doesn't work with nested classes.
   
   This class displays a window for editing and executing scripts.
   
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
class ScriptOutputDock : public QDockWidget
{
  /// Qt macro
  Q_OBJECT
  
public:
  /// Constructor
  ScriptOutputDock(const QString & title, ScriptManagerWidget *manager, QWidget *parent = 0, 
		   Qt::WindowFlags flags = 0);

  /// Is there anything here
  bool isEmpty() const;

public slots:
  /// Clear the text
  void clear();
  /// Print the text within the window
  void print();
  /// Save the output to a file
  void saveToFile();
  /// Change the title based on the script's execution state
  void setScriptIsRunning(bool running);    
	      
private slots:
  /// Context menu slot
  void showContextMenu(const QPoint & pos);
  /// A slot to pass messages to the output dock
  void displayOutputMessage(const QString &, bool error, bool timestamp);

private:
  /// Create the action pointers
  void initActions();
  /// Reset the default font
  void resetFont();

private:
  /// The script manager
  ScriptManagerWidget *m_manager;
  /// The actually widget that displays the text
  QTextEdit *m_text_display;
  /// Copy action
  QAction *m_copy;
};


/** @class ScriptingWindow    
    This class displays a seperate window for editing and executing scripts
*/
class ScriptingWindow : public QMainWindow
{
  ///Qt macro
  Q_OBJECT

public:
  ///Constructor
  ScriptingWindow(ScriptingEnv *env,QWidget *parent = 0, Qt::WindowFlags flags = 0);
  ///Destructor
  ~ScriptingWindow();
  /// Override the closeEvent
  void closeEvent(QCloseEvent *event);
  /// Override the hideEvent
  void hideEvent(QHideEvent *event);		      
  /// Override the showEvent
  void showEvent(QShowEvent *event);		      
  /// Is a script running?
  bool isScriptRunning() const;
  ///Save the current state of the script window for next time
  void saveSettings();
  /// Open a script in a new tab. This is here for backwards compatability with the old
  /// ScriptWindow class
  void open(const QString & filename, bool newtab = true);
  /// Execute all code. This is here for backwards compatability with the old
  /// ScriptWindow class
  void executeAll();
  ///saves scripts file names to a string 
  QString saveToString();
  ///Set whether to accept/reject close events
  void acceptCloseEvent(const bool value);

signals:
  /// Show the scripting language dialog
  void chooseScriptingLanguage();
  /// Tell others we are closing
  void closeMe();
  /// Tell others we are hiding
  void hideMe();
  
private:
  /// Create menu bar and menu actions
  void initMenus();
  /// Accept a custom defined event
  void customEvent(QEvent * event);

private slots:
  /// File menu is about to show
  void fileAboutToShow();
  /// Edit menu is about to show
  void editAboutToShow();
  /// Update window flags
  void updateWindowFlags();
  /// Update based on tab changes
  void tabSelectionChanged();

private:
  /// The script editors' manager
  ScriptManagerWidget *m_manager;
  /// Output display dock
  ScriptOutputDock *m_output_dock;

  /// File menu (actions are stored in ScriptManagerWidget)
  QMenu *m_file_menu;
  /// Edit menu
  QMenu *m_edit_menu;
  /// Specific window edit actions
  QAction *m_clear_output;
  /// Run menu (actions are stored in ScriptManagerWidget)
  QMenu *m_run_menu;
  /// Window menu
  QMenu *m_window_menu;
  /// Window actions
  QAction *m_always_on_top, *m_hide, *m_toggle_output, *m_print_output;
  /// Change scripting language
  QAction *m_scripting_lang;
  /// Flag to define whether we should accept a close event
  bool m_acceptClose;
  
};

#endif //SCRIPTINGWINDOW_H_
