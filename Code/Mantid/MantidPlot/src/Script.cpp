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

Script::Script(ScriptingEnv *env, const QString &name,
               const InteractionType interact, QObject * context)
  : QObject(), m_env(env), m_name(name), m_interactMode(interact), m_context(context),
    m_redirectOutput(true)
{
  m_env->incref();
}

Script::~Script()
{
  m_env->decref();
}

/**
 * @return True if the script is running
 */
bool Script::scriptIsRunning()
{
  return m_env->isRunning();
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

