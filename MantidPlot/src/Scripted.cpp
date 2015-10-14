//-------------------------------------
// Includes
//-------------------------------------
#include "Scripted.h"
#include "ScriptingEnv.h"

/**
 * Constructor
 * @param env :: A pointer to a scripting environment
 */
Scripted::Scripted(ScriptingEnv *env) :  m_scriptEnv(env)
{
  m_scriptEnv->incref();
}

/**
 * Desuctor
 */
Scripted::~Scripted()
{
  m_scriptEnv->decref();
}

/**
 * Called when the scripting environment changes
 * @param sce :: Scripting change event
 */
void Scripted::scriptingChangeEvent(ScriptingChangeEvent *sce)
{
  m_scriptEnv->decref();
  sce->scriptingEnv()->incref();
  m_scriptEnv = sce->scriptingEnv();
}
