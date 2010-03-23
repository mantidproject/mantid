#ifndef SCRIPTED_H_
#define SCRIPTED_H_

//------------------------------------------
// Includes
//------------------------------------------
#include <QEvent>
#include "customevents.h"

//-------------------------------------------
// Forward declarations
//------------------------------------------
class ScriptingEnv;

/**
 * A custom event to notify an object that it should update its scripting environment
 */
class ScriptingChangeEvent : public QEvent
{
public:
  ScriptingChangeEvent(ScriptingEnv *e) : QEvent(SCRIPTING_CHANGE_EVENT), env(e) {}
  ScriptingEnv *scriptingEnv() const { return env; }
  Type type() const { return SCRIPTING_CHANGE_EVENT; }
private:
  ScriptingEnv *env;
};

/**
 * An interface to the current scripting environment.
 *
 * Every class that wants to use a ScriptingEnv should subclass this one and
 * implement slot customEvent(QEvent*) such that it forwards any
 * ScriptingChangeEvents to Scripted::scriptingChangeEvent.
 */
class Scripted
{
  public:
  /// Constructor
  Scripted(ScriptingEnv* env);
  /// Destructor
  ~Scripted();
  /// Called when the scripting environent changes
  void scriptingChangeEvent(ScriptingChangeEvent*);
  /// Access the current environment
  ScriptingEnv *scriptingEnv(){return m_scriptEnv;}
  
  private:
  /// A pointer to the current environment
  ScriptingEnv *m_scriptEnv;
};

#endif // SCRIPTED_H_
