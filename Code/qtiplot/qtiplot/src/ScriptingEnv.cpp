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
#include "ScriptingEnv.h"
#include "Script.h"

#include <string.h>

#include <QDir>
#include <QDateTime>
#include <Qsci/qscilexer.h> //Mantid
#include <Qsci/qsciapis.h> //Mantid
#include "MantidKernel/ConfigService.h" //Mantid

#ifdef SCRIPTING_MUPARSER
#include "muParserScript.h"
#include "muParserScripting.h"
#endif
#ifdef SCRIPTING_PYTHON
#include "PythonScript.h"
#include "PythonScripting.h"
#endif

ScriptingEnv::ScriptingEnv(ApplicationWindow *parent, const char *langName)
  : QObject(0, langName), d_parent(parent), languageName(langName), m_report_progress(false), 
  m_is_running(false), m_current_script(NULL), m_lexer(NULL), m_completer(NULL), m_api_preparing(false)
{
  d_initialized=false;
  d_refcount=0;
}

ScriptingEnv::~ScriptingEnv()
{
  if( m_completer )
  {
    delete m_completer;
  }
  if( m_lexer )
  { 
    delete m_lexer;
  }
}

bool ScriptingEnv::initialize()
{
  static bool is_initialized(false);
  if( !is_initialized )
  {
    is_initialized = true;
    return start();
  }
  return true;
}

//Mantid
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
/**
 * Set the code lexer for this environment
 */
void ScriptingEnv::setCodeLexer(QsciLexer* lexer)
{ 
  if( m_lexer )
  {
    delete m_lexer;
    m_lexer = NULL;
  }

  m_lexer = lexer;
  // Get the API for this environment
  if( m_completer == NULL )
  {
    m_completer = new QsciAPIs(m_lexer);
    connect(m_completer,SIGNAL(apiPreparationStarted()), this, SLOT(apiPrepStarted()));
    connect(m_completer,SIGNAL(apiPreparationFinished()), this, SLOT(apiPrepDone()));
  }
}

void ScriptingEnv::updateCodeCompletion(const QString & fname, bool prepare)
{
  if( m_completer->load(fname) && prepare) 
  {
    // This is performed in a separate thread as it can take a while
    if( !m_api_preparing )
    {
      m_completer->prepare();
    }
  }
}

// Slot to handle the started signal from the api auto complete preparation
void ScriptingEnv::apiPrepStarted()
{
  m_api_preparing = true;
}

// Slot to handle the completed signal from the api auto complete preparation
void ScriptingEnv::apiPrepDone()
{
  m_api_preparing = false;
}

void ScriptingEnv::execute(const QString & code)
{
  m_current_script->setCode(code);
  m_current_script->exec();
}						
