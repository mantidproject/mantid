// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------
// Includes
//-------------------------------------
#include "Scripted.h"
#include "ScriptingEnv.h"

/**
 * Constructor
 * @param env :: A pointer to a scripting environment
 */
Scripted::Scripted(ScriptingEnv *env) : m_scriptEnv(env) {
  m_scriptEnv->incref();
}

/**
 * Desuctor
 */
Scripted::~Scripted() { m_scriptEnv->decref(); }

/**
 * Called when the scripting environment changes
 * @param sce :: Scripting change event
 */
void Scripted::scriptingChangeEvent(ScriptingChangeEvent *sce) {
  m_scriptEnv->decref();
  sce->scriptingEnv()->incref();
  m_scriptEnv = sce->scriptingEnv();
}
