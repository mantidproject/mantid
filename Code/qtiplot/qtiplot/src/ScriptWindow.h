/***************************************************************************
    File                 : ScriptWindow.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, 
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Python script window
                           
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
#ifndef SCRIPTWINDOW_H
#define SCRIPTWINDOW_H

/**
  * Mantid - This class has been modified from the qtiplot class so that
  * the output goes into a seperate area within the window
  */

#include "ScriptEdit.h"

#include <QMainWindow>
#include <QMenu>
#include <QCloseEvent>
#include <QTextEdit>
class ScriptingEnv;
class ApplicationWindow;
class QAction;
class QDockWidget;

class OutputTextArea; //Mantid

//! Python script window
class ScriptWindow: public QMainWindow
{
	Q_OBJECT

public:
		ScriptWindow(ScriptingEnv *env, ApplicationWindow *app);
    virtual ~ScriptWindow();						       
    void customEvent(QEvent*);
    void askSave();
    ScriptEdit* scriptEditor() { return te; };			     

public slots:
		void newScript();
		void open(const QString& fn = QString());
		void save();
		void saveAs();
		void languageChange();
		virtual void setVisible(bool visible);

		void executeAll(){te->executeAll();};

                void scriptMessage(const QString&);				    
		void scriptError(const QString&);
                void insertOutputSeparator();
                void updateWindowTitle();

                void setEditEnabled(bool);

private slots:
		void setAlwaysOnTop(bool on);
		void viewScriptOutput(bool on);
                void editChanged();
                void executionStateChange(bool active);

signals:
		void visibilityChanged(bool visible);

private:
		void moveEvent( QMoveEvent* );
		void resizeEvent( QResizeEvent* );

		void initMenu();
		void initActions();
		ScriptEdit *te;
                QDockWidget* outputWindow;
                OutputTextArea* outputText;

                ScriptingEnv* d_env;
		ApplicationWindow *d_app;

		QString fileName;
                bool fileSaved;

		QMenu *file, *edit, *run, *windowMenu;
		QAction *actionNew, *actionUndo, *actionRedo, *actionCut, *actionCopy, *actionPaste;
                QAction *actionClearOutput, *actionClearInput;
		QAction *actionExecute, *actionExecuteAll, *actionEval, *actionOpen;
		QAction *actionSave, *actionSaveAs;
		QAction *actionAlwaysOnTop, *actionHide;
		QAction *actionViewScriptOutput;
                QAction *actionPrintInput, *actionPrintOutput;
  
                // These need to be stored so that they can be enabled/disabled when the
                //corresponding actions are toggled since Qt < v4.5 doesn't do this itself
                QList<QKeySequence> executeShortcuts;
};

//Mantid - This class is here so that the context menu handler can be overridden but it can't be nested in the private section of 
//ScriptWindow because the SIGNAL/SLOT mechanism won't work
class OutputTextArea : public QTextEdit
  {
    Q_OBJECT

    public:

    OutputTextArea(QWidget * parent = 0, const char * name = 0 );

  public slots:
    
    void printOutput();

    protected:
    
    virtual void contextMenuEvent(QContextMenuEvent *e);
};

#endif
