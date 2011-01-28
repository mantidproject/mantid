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
#include "ScriptingEnv.h"
#include "Script.h"

#include <QMessageBox>
#include <QRegExp>

Script::Script(ScriptingEnv *env, const QString &code, bool interactive, QObject *context, 
	       const QString &name)
  : Env(env), Code(code), Name(name), isInteractive(interactive), compiled(notCompiled), 
    m_line_offset(-1)
{ 
  Env->incref(); 
  Context = context; 
  EmitErrors=true; 
}

Script::~Script()
{
  Env->decref();
}

void Script::addCode(const QString &code) 
{ 
  Code.append(normaliseLineEndings(code));
  compiled = notCompiled; 
  emit codeChanged(); 
}

void Script::setCode(const QString &code) 
{
  Code = normaliseLineEndings(code);
  compiled = notCompiled; 
  emit codeChanged();
}


bool Script::compile(bool)
{
  emit_error("Script::compile called!", 0);
  return false;
}

QVariant Script::eval()
{
  emit_error("Script::eval called!",0);
  return QVariant();
}

bool Script::exec()
{
  emit_error("Script::exec called!",0);
  return false;
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

