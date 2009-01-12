/***************************************************************************
    File                 : ScriptWindow.cpp
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

 
/**
  * Mantid - This class has been modified from the qtiplot class so that
  * the output goes into a seperate window.
  */
 
#include "ScriptWindow.h"
#include "ApplicationWindow.h"
#include "ScriptEdit.h"
#include "pixmaps.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPixmap>
#include <QCloseEvent>
#include <QTextStream>
#include <QDockWidget>
#include <QTextEdit>
#include <QPrintDialog>

ScriptWindow::ScriptWindow(ScriptingEnv *env, ApplicationWindow *app)
  : QMainWindow(), d_env(env), d_app(app), fileName(QString::null), fileSaved(false)
{	
  //---------- Mantid ---------------
	outputWindow = new QDockWidget(this);
	outputWindow->setObjectName(tr("outputWindow"));
	outputWindow->setWindowTitle(tr("Script Output"));
	outputWindow->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
	
	addDockWidget( Qt::BottomDockWidgetArea, outputWindow );
	
	outputText = new OutputTextArea(outputWindow);
	outputWindow->setWidget(outputText);
	outputText->setMinimumHeight(25);
	outputWindow->setMinimumHeight(25);
  //---------------------------------
	


	te = new ScriptEdit(env, this, name());
	te->setContext(this);
	te->setDirPath(d_app->scriptsDirPath);
	te->resize(600,300);
	connect(te, SIGNAL(dirPathChanged(const QString&)), d_app, SLOT(scriptsDirPathChanged(const QString&)));
	connect(te, SIGNAL(outputMessage(const QString&)), this, SLOT(scriptMessage(const QString&)));
	connect(te, SIGNAL(outputError(const QString&)), this, SLOT(scriptError(const QString&)));
	connect(te, SIGNAL(textChanged()), this, SLOT(editChanged()));
	setCentralWidget(te);

	initMenu();

  //Mantid - Moved this to after output and scriptedit have been created
	initActions();

	setIcon(QPixmap(logo_xpm));

	//updateWindowTitle(env->scriptingLanguage());
  setWindowTitle("MantidPlot: " + env->scriptingLanguage() + " Window - New File");

	setFocusProxy(te);
	setFocusPolicy(Qt::StrongFocus);
}

ScriptWindow::~ScriptWindow()
{
 // ------Mantid -------------
  if( outputText ) 
    delete outputText;
  if( outputWindow ) 
    delete outputWindow;
  // -------------------------
    if( te )
    delete te;
}

void ScriptWindow::customEvent(QEvent *e)
{
  if (e->type() == SCRIPTING_CHANGE_EVENT)
    {
      outputText->clear();
      ScriptingChangeEvent* event = (ScriptingChangeEvent*)e;
      d_env = event->scriptingEnv();
      updateWindowTitle();
    }
}

void ScriptWindow::closeEvent(QCloseEvent* event)
{
  if( !fileSaved && !te->text().isEmpty() )
  {
    askSave();
  }
  event->accept();
}

void ScriptWindow::askSave()
{
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("MantidPlot");
  msgBox.setText(tr("The script has been modified."));
  msgBox.setInformativeText(tr("Save changes?"));
  msgBox.addButton(QMessageBox::Save);
  QPushButton *saveAsButton = msgBox.addButton("Save As...", QMessageBox::AcceptRole);
  msgBox.addButton(QMessageBox::Discard);
  int ret = msgBox.exec();
  if( msgBox.clickedButton() == saveAsButton ) 
  {
     saveAs();
  }
  else if( ret == QMessageBox::Save )
  {
     save();
  }
  else return;
}

void ScriptWindow::initMenu()
{
	file = new QMenu(tr("&File"), this);
	menuBar()->addMenu(file);

	edit = new QMenu(tr("&Edit"), this);
	menuBar()->addMenu(edit);

	run = new QMenu(tr("E&xecute"), this);
	menuBar()->addMenu(run);

	windowMenu = new QMenu(tr("&Window"), this);
	menuBar()->addMenu(windowMenu);	
}

void ScriptWindow::initActions()
{
  //File Actions
	actionNew = new QAction(QPixmap(new_xpm), tr("&New"), this);
	actionNew->setShortcut( tr("Ctrl+N") );
	connect(actionNew, SIGNAL(activated()), this, SLOT(newScript()));
	file->addAction(actionNew);

	actionOpen = new QAction(QPixmap(fileopen_xpm), tr("&Open..."), this);
	actionOpen->setShortcut( tr("Ctrl+O") );
	connect(actionOpen, SIGNAL(activated()), this, SLOT(open()));
	file->addAction(actionOpen);

	actionSave = new QAction(QPixmap(filesave_xpm), tr("&Save"), this);
	actionSave->setShortcut( tr("Ctrl+S") );
	connect(actionSave, SIGNAL(activated()), this, SLOT(save()));
	file->addAction(actionSave);

	actionSaveAs = new QAction(tr("Save &As..."), this);
	connect(actionSaveAs, SIGNAL(activated()), this, SLOT(saveAs()));
	file->addAction(actionSaveAs);

	actionPrintInput = new QAction(QPixmap(fileprint_xpm), tr("&Print Input ..."), this);
	actionPrintInput->setShortcut( tr("Ctrl+P") );
	connect(actionPrintInput, SIGNAL(activated()), te, SLOT(print()));
	file->addAction(actionPrintInput);

	actionPrintOutput = new QAction(QPixmap(fileprint_xpm), tr("&Print Output ..."), this);
	connect(actionPrintOutput, SIGNAL(activated()), outputText, SLOT(printOutput()));
	file->addAction(actionPrintOutput);

  //Edit Actions

	actionUndo = new QAction(QPixmap(undo_xpm), tr("&Undo"), this);
	actionUndo->setShortcut( tr("Ctrl+Z") );
	connect(actionUndo, SIGNAL(activated()), te, SLOT(undo()));	
 	edit->addAction(actionUndo);
 	actionUndo->setEnabled(false);

	actionRedo = new QAction(QPixmap(redo_xpm), tr("&Redo"), this);
	actionRedo->setShortcut( tr("Ctrl+Y") );
	connect(actionRedo, SIGNAL(activated()), te, SLOT(redo()));	
 	edit->addAction(actionRedo);
 	actionRedo->setEnabled(false);

 	edit->insertSeparator();

	actionCut = new QAction(QPixmap(cut_xpm), tr("&Cut"), this);
	actionCut->setShortcut( tr("Ctrl+x") );
	connect(actionCut, SIGNAL(activated()), te, SLOT(cut()));	
	edit->addAction(actionCut);
	actionCut->setEnabled(false);

	actionCopy = new QAction(QPixmap(copy_xpm), tr("&Copy"), this);
	actionCopy->setShortcut( tr("Ctrl+C") );
	connect(actionCopy, SIGNAL(activated()), te, SLOT(copy()));	
	edit->addAction(actionCopy);
	actionCopy->setEnabled(false);

	actionPaste = new QAction(QPixmap(paste_xpm), tr("&Paste"), this);
	actionPaste->setShortcut( tr("Ctrl+V") );
	connect(actionPaste, SIGNAL(activated()), te, SLOT(paste()));	
	edit->addAction(actionPaste);

	edit->insertSeparator();
	
	actionClearOutput = new QAction(tr("&Clear Output"), this);
	connect(actionClearOutput, SIGNAL(activated()), outputText, SLOT(clear()));
	edit->addAction(actionClearOutput);
	
   // Run actions
	actionExecute = new QAction(tr("E&xecute"), this);
	actionExecute->setShortcut( tr("Ctrl+Return") );
	connect(actionExecute, SIGNAL(activated()), te, SLOT(execute()));
	run->addAction(actionExecute);

	actionExecuteAll = new QAction(tr("Execute &All"), this);
	actionExecuteAll->setShortcut( tr("Ctrl+Shift+Return") );
	connect(actionExecuteAll, SIGNAL(activated()), te, SLOT(executeAll()));
	run->addAction(actionExecuteAll);

	actionEval = new QAction(tr("&Evaluate Expression"), this);
	actionEval->setShortcut( tr("Ctrl+E") );
	connect(actionEval, SIGNAL(activated()), te, SLOT(evaluate()));
	run->addAction(actionEval);

  //Window actions
	actionAlwaysOnTop = new QAction(tr("Always on &Top"), this);
	actionAlwaysOnTop->setCheckable(true);
	if (d_app)
		actionAlwaysOnTop->setChecked (d_app->d_script_win_on_top);
	windowMenu->addAction(actionAlwaysOnTop);
	connect(actionAlwaysOnTop, SIGNAL(toggled(bool)), this, SLOT(setAlwaysOnTop(bool)));

	actionHide = new QAction(tr("&Hide"), this);
	connect(actionHide, SIGNAL(activated()), this, SLOT(close()));
	windowMenu->addAction(actionHide);

	actionViewScriptOutput = outputWindow->toggleViewAction();
	actionViewScriptOutput->setText("&Show Script Window");
	actionViewScriptOutput->setChecked(true);
	windowMenu->addAction(actionViewScriptOutput);	
	
	connect(te, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
	connect(te, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));
	connect(te, SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
	connect(te, SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));

}

void ScriptWindow::languageChange()
{
        updateWindowTitle(); //Mantid

	menuBar()->clear();
	menuBar()->addMenu(file);
	menuBar()->addMenu(edit);
	menuBar()->addMenu(run);

	file->setTitle(tr("&File"));
	edit->setTitle(tr("&Edit"));
	run->setTitle(tr("E&xecute"));

	menuBar()->addAction(tr("&Hide"), this, SLOT(close()));

	actionNew->setText(tr("&New"));
	actionNew->setShortcut(tr("Ctrl+N"));

	actionOpen->setText(tr("&Open..."));
	actionOpen->setShortcut(tr("Ctrl+O"));

	actionSave->setText(tr("&Save"));
	actionSave->setShortcut(tr("Ctrl+S"));

	actionSaveAs->setText(tr("Save &As..."));

	actionUndo->setText(tr("&Undo"));
	actionUndo->setShortcut(tr("Ctrl+Z"));

	actionRedo->setText(tr("&Redo"));
	actionRedo->setShortcut(tr("Ctrl+Y"));

	actionCut->setText(tr("&Cut"));
	actionCut->setShortcut(tr("Ctrl+x"));

	actionCopy->setText(tr("&Copy"));
	actionCopy->setShortcut(tr("Ctrl+C"));

	actionPaste->setText(tr("&Paste"));
	actionPaste->setShortcut(tr("Ctrl+V"));

	actionExecute->setText(tr("E&xecute"));
	actionExecute->setShortcut(tr("CTRL+J"));

	actionExecuteAll->setText(tr("Execute &All"));
	actionExecuteAll->setShortcut(tr("CTRL+SHIFT+J"));

	actionEval->setText(tr("&Evaluate Expression"));
	actionEval->setShortcut(tr("CTRL+Return"));
}


void ScriptWindow::updateWindowTitle()
{
  QString title("MantidPlot: " + d_env->scriptingLanguage() + " Window - ");
  if( fileName.isNull() || fileName.isEmpty() )
  {
    title += "New File";
  }
  else
  {
    title += fileName;//QFileInfo(fileName).fileName();
  }
  if( !fileSaved && !te->text().isEmpty() ) title += " (unsaved)";
  setWindowTitle(title);
}

void ScriptWindow::newScript()
{
  if( !fileSaved ) save();
	fileName = QString::null;
	te->clear();
	updateWindowTitle();
}

void ScriptWindow::open(const QString& fn)
{
	QString s = te->importASCII(fn);
	//Mantid
	if (!s.isEmpty()) fileName = s;
	fileSaved = true;
	updateWindowTitle();
}

void ScriptWindow::saveAs()
{
	QString fn = te->exportASCII();
	if (!fn.isEmpty())
		fileName = fn;
	fileSaved = true;
	updateWindowTitle();
}

void ScriptWindow::save()
{
	if (!fileName.isEmpty()){
		QFile f(fileName);
		if ( !f.open( QIODevice::WriteOnly ) ){
			QMessageBox::critical(0, tr("MantidPlot - File Save Error"),
					tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fileName));
			return;
		}
		QTextStream t( &f );
		t.setCodec("UTF-8");
		t << te->text();
		f.close();
	} else
		saveAs();

	fileSaved = true;
	updateWindowTitle();
}

void ScriptWindow::setVisible(bool visible)
{
	if (visible == isVisible())
		return;
	QMainWindow::setVisible(visible);
	emit visibilityChanged(visible);
}

void ScriptWindow::setAlwaysOnTop(bool on)
{	
	if (!d_app)
		return;
	
	d_app->d_script_win_on_top = on;
	
	QString msg = tr("You need to close and reopen the script window before your changes become effective! Do you want to close it now?");
	if (QMessageBox::question(this, tr("QtiPlot"), msg, QMessageBox::Ok, QMessageBox::No) == QMessageBox::Ok)
		this->close();
}

void ScriptWindow::moveEvent( QMoveEvent* e )
{
	d_app->d_script_win_rect = QRect(pos(), size());
	e->accept();
}

void ScriptWindow::resizeEvent( QResizeEvent* e )
{
	d_app->d_script_win_rect = QRect(geometry().topLeft(), size());
	e->accept();
}

void ScriptWindow::scriptMessage(const QString& text)
{
  outputText->setTextColor(Qt::black);
  outputText->textCursor().insertText(text);
  outputText->moveCursor(QTextCursor::End);
}				    

void ScriptWindow::scriptError(const QString& text)
{
  outputText->setTextColor(Qt::red);
  outputText->textCursor().insertText(text);
  outputText->moveCursor(QTextCursor::End);
}				    

void ScriptWindow::viewScriptOutput(bool visible)
{
  outputWindow->setVisible(visible);
}

void ScriptWindow::editChanged()
{
  fileSaved = false;
  updateWindowTitle();
}


OutputTextArea::OutputTextArea(QWidget * parent, const char * name) : QTextEdit(parent, name)
{
  setReadOnly(true);
}

void OutputTextArea::contextMenuEvent(QContextMenuEvent *e)
{
  QMenu menu(this);
  
  QAction* clear = new QAction("Clear", this);
  connect(clear, SIGNAL(activated()), this, SLOT(clear()));
  menu.addAction(clear);

  QAction* copy = new QAction(QPixmap(copy_xpm), "Copy", this);
  connect(copy, SIGNAL(activated()), this, SLOT(copy()));
  menu.addAction(copy);

  if( !document()->isEmpty() )
  {
    QAction* print = new QAction(QPixmap(fileprint_xpm), "Print", this);
    connect(print, SIGNAL(activated()), this, SLOT(printOutput()));
    menu.addAction(print);
  }
  
  menu.exec(e->globalPos());
}

void OutputTextArea::printOutput()
{
  QTextDocument* doc = document(); 
  QPrinter printer;
  printer.setColorMode(QPrinter::GrayScale);
  printer.setCreator("MantidPlot");
  QPrintDialog printDialog(&printer);
  printDialog.setWindowTitle("MantidPlot - Print Script Output");
  if (printDialog.exec() == QDialog::Accepted) 
  {
    doc->print(&printer);
  }
}
