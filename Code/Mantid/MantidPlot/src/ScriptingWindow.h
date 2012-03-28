#ifndef SCRIPTINGWINDOW_H_
#define SCRIPTINGWINDOW_H_

//----------------------------------
// Includes
//----------------------------------
#include <QMainWindow>

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


/** @class ScriptingWindow    
    This class displays a seperate window for editing and executing scripts
*/
class ScriptingWindow : public QMainWindow
{
  ///Qt macro
  Q_OBJECT

public:
  ///Constructor
  ScriptingWindow(ScriptingEnv *env,bool capturePrint = true,QWidget *parent = 0, Qt::WindowFlags flags = 0);
  ///Destructor
  ~ScriptingWindow();
  /// Override the closeEvent
  void closeEvent(QCloseEvent *event);
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
  /// Create window actions
  void initWindowActions();

  /// Accept a custom defined event
  void customEvent(QEvent * event);

private slots:
  /// File menu is about to show
  void fileMenuAboutToShow();
  /// Edit menu is about to show
  void editMenuAboutToShow();
  /// Exec menu about to show
  void execMenuAboutToShow();
  /// Window menu is about to show
  void windowMenuAboutToShow();

  /// Update window flags
  void updateWindowFlags();
  /// Update based on tab changes
  void tabSelectionChanged();

private:
  /// The script editors' manager
  ScriptManagerWidget *m_manager;

  /// File menu
  QMenu *m_file_menu;
  /// Edit menu
  QMenu *m_edit_menu;
  /// Run menu
  QMenu *m_run_menu;
  /// Window menu
  QMenu *m_window_menu;
  /// Window actions
  QAction *m_always_on_top, *m_hide;
  /// Change scripting language
  QAction *m_scripting_lang;
  /// Flag to define whether we should accept a close event
  bool m_acceptClose;
};

#endif //SCRIPTINGWINDOW_H_
