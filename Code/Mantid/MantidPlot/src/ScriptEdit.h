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

// Mantid - This class has been rewritten to include QsciScintilla and modified so that
// the text output no longer gets sent to the editor window. Most changes to qtiplot should
// not be merged here - M. Gigg

#include "ScriptingEnv.h"
#include "Scripted.h"
#include "Script.h"

// Qt includes
#include <QMenu>
#include <QEvent>

#include <Qsci/qsciscintilla.h>

class QAction;
class QMenu;
class QsciLexer;


/*!\brief Editor widget with support for evaluating expressions and executing code.
 *
 * Inherits from QsciScintilla, which supports syntax highlighting and line numbering
 */
class ScriptEdit: public QsciScintilla, public Scripted
{
  Q_OBJECT

  public:
  ScriptEdit(ScriptingEnv *env, QWidget *parent=0, const char *name=0);
  ~ScriptEdit();
  //! Handle changing of scripting environment.
  void customEvent(QEvent*);
  //! Map cursor positions to line numbers.
  int lineNumber() const;
			
  bool isRunning() const { return m_bIsRunning; }

public slots:
  void execute();
  void executeAll();
  void evaluate();
  
  void print();
  void exportPDF(const QString& fileName);
  void importCodeBlock(const QString & code);
  QString exportASCII(const QString &file=QString::null);
  QString importASCII(const QString &file=QString::null);
  void insertFunction(const QString &);
  void insertFunction(QAction * action);
  void setContext(QObject *context) { myScript->setContext(context); }
  void scriptPrint(const QString&);
  void setDirPath(const QString& path);

  void updateEditor();

  void setExecuteActionsEnabled(bool);
  QString outputSeparator();
  void updateLineMarker(int);

  void runScript(const QString & code);

  signals:
  void outputMessage(const QString& text);
  void outputError(const QString& text);
  void dirPathChanged(const QString& path);

  void undoAvailable(bool available);
  void redoAvailable(bool available);
  
  void ScriptIsActive(bool active);
  void abortExecution();

  protected:
  virtual void contextMenuEvent(QContextMenuEvent *e);

  private:
  Script *myScript;
  QAction *actionExecute, *actionExecuteAll, *actionEval, *actionAbort;
  //! Submenu of context menu with mathematical functions.
  QMenu *functionsMenu;
  //! Cursor used for output of evaluation results and error messages.  
  QString scriptsDirPath;
  QsciLexer* codeLexer;
  //! True if we are inside evaluate(), execute() or executeAll() there were errors.
  bool d_error;

  //An integer defining the handle to a code marker     
  int m_iCodeMarkerHandle;
  int m_iFirstLineNumber;

  bool m_bIsRunning;
  bool m_bErrorRaised;		   

  private slots:
  //! Insert an error message from the scripting system at printCursor.
  /**
   * After insertion, the text cursor will have the error message selected, allowing the user to
   * delete it and fix the error.
   */
  void insertErrorMsg(const QString &message);

};


#endif
