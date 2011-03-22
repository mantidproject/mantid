/***************************************************************************
    File                 : ScriptingEnv.cpp
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
// M. Gigg. Python is slightly special in that is better to include it before 
// any system headers are included.
#ifdef SCRIPTING_PYTHON
#include "PythonScript.h"
#include "PythonScripting.h"
#endif

#ifdef SCRIPTING_MUPARSER
#include "muParserScript.h"
#include "muParserScripting.h"
#endif

#include "ScriptingEnv.h"
#include "Script.h"

#include <string.h>

#include <QDir>
#include <QDateTime>
#include "MantidKernel/ConfigService.h"


ScriptingEnv::ScriptingEnv(ApplicationWindow *parent, const char *langName)
  : QObject(0, langName), d_initialized(false), d_parent(parent), m_is_running(false), d_refcount(0),
    languageName(langName) 
{
}

ScriptingEnv::~ScriptingEnv()
{
}

bool ScriptingEnv::initialize()
{
  static bool init_called(false);
  if( !init_called )
  {
    init_called = true;
    return start();
  }
  return isInitialized();
}

const QString ScriptingEnv::scriptingLanguage() const
{
  return QString(languageName);
}

const QString ScriptingEnv::fileFilter() const
{
  QStringList extensions = fileExtensions();
  if (extensions.isEmpty())
    return "";
  else
    return tr("%1 Source (*.%2);;").arg(name()).arg(extensions.join(" *."));
}

void ScriptingEnv::incref()
{
  d_refcount++;
}

void ScriptingEnv::decref()
{
  d_refcount--;
  if (d_refcount==0)
    delete this;
}

//---------------------------------------------------------
// ScriptingLangManager
//---------------------------------------------------------
/**
 * A list of available languages
 */
ScriptingLangManager::ScriptingLang ScriptingLangManager::g_langs[] = 
  {
#ifdef SCRIPTING_MUPARSER
    { muParserScripting::langName, muParserScripting::constructor },
#endif
#ifdef SCRIPTING_PYTHON
    { PythonScripting::langName, PythonScripting::constructor },
#endif
    // Sentinel defining the end of the list
    { NULL, NULL }
};

ScriptingEnv *ScriptingLangManager::newEnv(ApplicationWindow *parent)
{
  if (!g_langs[0].constructor)
  {
    return NULL;
  }
  else 
  {
    return g_langs[0].constructor(parent);
  }
}

ScriptingEnv *ScriptingLangManager::newEnv(const char *name, ApplicationWindow *parent)
{
  for(ScriptingLang *l = g_langs; l->constructor; l++)
  {	
    if( QString(name) == QString(l->name) )
    {
      return l->constructor(parent);
    }
  }
  return NULL;
}

QStringList ScriptingLangManager::languages()
{
  QStringList lang_list;
  for (ScriptingLang *l = g_langs; l->constructor; l++)
  {
    lang_list << l->name;
  }
  return lang_list;
}
