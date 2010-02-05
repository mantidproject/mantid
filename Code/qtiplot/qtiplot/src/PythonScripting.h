/***************************************************************************
	File                 : PythonScripting.h
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2006 by Knut Franke
	Email (use @ for *)  : knut.franke*gmx.de
	Description          : Execute Python code from within QtiPlot

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
#ifndef PYTHON_SCRIPTING_H
#define PYTHON_SCRIPTING_H

#include "ScriptingEnv.h"
#include "PythonScript.h"
#include <Qsci/qscilexerpython.h> //Mantid

class QObject;
class QString;

class PythonScripting: public ScriptingEnv
{
  Q_OBJECT
  
  public:
  /// The langauge name
  static const char *langName;
  //Factory function
  static ScriptingEnv *constructor(ApplicationWindow *parent);
  /// Default constructor
  PythonScripting(ApplicationWindow *parent);
  ///Destructor
  ~PythonScripting();

  void write(const QString &text) { emit print(text); }
  
  //Mantid. To ensure the correct code lexer for Python 
  virtual QsciLexer* scriptCodeLexer() const { return new QsciLexerPython; }

  // Python supports progress monitoring
  virtual bool supportsProgressReporting() const { return true; }
  
  //! like str(object) in Python
  /**
   * Convert object to a string.
   * Steals a reference to object if decref is true; borrows otherwise.
   */
  QString toString(PyObject *object, bool decref=false);
  // Create a new script object that can execute code within this enviroment
  Script *newScript(const QString &code, QObject *context, const QString &name="<input>")
  {
    m_current_script = new PythonScript(this, code, context, name);
    return m_current_script;
  }

  bool setQObject(QObject*, const char*, PyObject *dict);
  bool setQObject(QObject *val, const char *name) { return setQObject(val,name,NULL); }
  bool setInt(int, const char*, PyObject *dict=NULL);
  bool setDouble(double, const char*, PyObject *dict=NULL);

  const QStringList mathFunctions() const;
  const QString mathFunctionDoc (const QString &name) const;
  const QStringList fileExtensions() const;

  PyObject *globalDict() { return m_globals; }
  PyObject *sysDict() { return m_sys; }

  public slots:
    virtual void refreshAlgorithms();


private:
  //Start the environment
  bool start();
  //Shutdown the environment
  void shutdown();
  //Load a file
  bool loadInitFile(const QString &path);

  PyObject *m_globals;		// PyDict of global environment
  PyObject *m_math;		// PyDict of math functions
  PyObject *m_sys;		// PyDict of sys module
};

#endif
