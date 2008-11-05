/***************************************************************************
    File                 : ScriptEdit.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Scripting classes

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef SCRIPTEDIT_H
#define SCRIPTEDIT_H

#include "ScriptingEnv.h"
#include "Script.h"

#include <QMenu>
#include <QTextEdit>
#include <QThread>

class QAction;
class QMenu;

//Mantid
class ExecuteThread;

/*!\brief Editor widget with support for evaluating expressions and executing code.
 *
 * \section future Future Plans
 * - Display line numbers.
 * - syntax highlighting, indentation, auto-completion etc. (maybe using QScintilla)
 */
class ScriptEdit: public QTextEdit, public scripted
{
  Q_OBJECT

  public:
    ScriptEdit(ScriptingEnv *env, QWidget *parent=0, const char *name=0);
    ~ScriptEdit();
	//! Handle changing of scripting environment.
    void customEvent(QEvent*);
  	//! Map cursor positions to line numbers.
    int lineNumber(int pos) const;

  public slots:
    void executeAsync();
    void executeAllAsync();
    void execute();
    void executeAll();
    void evaluate();
    void print();
    void exportPDF(const QString& fileName);
    QString exportASCII(const QString &file=QString::null);
    QString importASCII(const QString &file=QString::null);
    void insertFunction(const QString &);
    void insertFunction(QAction * action);
    void setContext(QObject *context) { myScript->setContext(context); }
    void scriptPrint(const QString&);
    void updateIndentation();
	void setDirPath(const QString& path);

  signals:
	void dirPathChanged(const QString& path);
	
  protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);

  private:
    ExecuteThread *exThread;
    Script *myScript;
    QAction *actionExecute, *actionExecuteAll, *actionEval, *actionPrint, *actionImport, *actionExport;
    //! Submenu of context menu with mathematical functions.
  	QMenu *functionsMenu;
  	//! Cursor used for output of evaluation results and error messages.  
  	QTextCursor printCursor;
  	QString scriptsDirPath;
  
   //! Format used for resetting success/failure markers.
	QTextBlockFormat d_fmt_default;
	//! Format used for marking code that was executed or evaluated successfully.
	QTextBlockFormat d_fmt_success;
	//! Format used for marking code that resulted in an error.
	QTextBlockFormat d_fmt_failure;
	//! True if we are inside evaluate(), execute() or executeAll() there were errors.
	bool d_error;

  private slots:
	  //! Insert an error message from the scripting system at printCursor.
		/**
		* After insertion, the text cursor will have the error message selected, allowing the user to
		* delete it and fix the error.
		*/
  void insertErrorMsg(const QString &message);

};

//Mantid
/// Thread to execute the script asynchronously.
class ExecuteThread:public QThread
{
    ScriptEdit *edit;
    bool all;
public:
    ExecuteThread(ScriptEdit *sedit,bool yes=false):edit(sedit),all(yes){}
    void run();
    void stop();
};

#endif
