/***************************************************************************
    File                 : Script.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Knut Franke
    Email (use @ for *)  : knut.franke*gmx.de
    Description          : Implementations of generic scripting classes
                           
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
#include "Script.h"
#include "ScriptingEnv.h"
#include <QRegExp>

#include <QtConcurrentRun>


Script::Script(ScriptingEnv *env, const QString &name,
               const InteractionType interact, QObject * context)
  : QObject(), m_env(env), m_name() , m_context(context),
    m_redirectOutput(true), m_reportProgress(false), m_interactMode(interact),
    m_execMode(NotExecuting), m_execThread(new ScriptThread(*this))
{
  m_env->incref();

  setName(name);

  connect(this, SIGNAL(started(const QString &)), this, SLOT(setIsRunning()));
  connect(this, SIGNAL(finished(const QString &)), this, SLOT(setNotExecuting()));
  connect(this, SIGNAL(error(const QString &, const QString &, int)), this, SLOT(setNotExecuting()));
}

Script::~Script()
{
  delete m_execThread;
  m_env->decref();
}

/**
 * Sets a new name for the script
 */
void Script::setName(const QString & name)
{ 
  m_name = name.toStdString();
}

/**
 * Compile the code, returning true/false depending on the status
 * @param code Code to compile
 * @return True/false depending on success
 */
bool Script::compile(const ScriptCode & code)
{
  setupCode(code);
  return this->compileImpl();
}

/**
 * Evaluate the Code, returning QVariant() on an error / exception.
 * @param code Code to evaluate
 * @return The result as a QVariant
 */
QVariant Script::evaluate(const ScriptCode & code)
{
  setupCode(code);
  return this->evaluateImpl();
}

/// Execute the Code, returning false on an error / exception.
bool Script::execute(const ScriptCode & code)
{
  setupCode(code);
  emit started("");
  return this->executeImpl();
}

/// Execute the code asynchronously, returning immediately after the execution has started
QFuture<bool> Script::executeAsync(const ScriptCode & code)
{
  setupCode(code);
  emit started("");
  return QtConcurrent::run(this, &Script::executeImpl);
  //m_execThread->start();
  //return m_execThread->future();
}


/// Sets the execution mode to NotExecuting
void Script::setNotExecuting()
{
  m_execMode = NotExecuting;
}

/// Sets the execution mode to Running to indicate something is running
void Script::setIsRunning()
{
  m_execMode = Running;
}

/**
 * Sets the offset & code string
 */
void Script::setupCode(const ScriptCode & code)
{
  m_code = code;
}

/**
 * Ensure that any line endings are converted to single '\n' so that the Python C API is happy
 * @param text :: The text to check and convert
 */
QString Script::normaliseLineEndings(QString text) const
{
  text = text.replace(QRegExp("\\r\\n"), QString("\n"));
  text = text.replace(QRegExp("\\r"), QString("\n"));
  return text;
}

