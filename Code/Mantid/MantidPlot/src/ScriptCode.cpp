#include "ScriptCode.h"

/// Empty code
ScriptCode::ScriptCode() : m_code(), m_offset()
{
}

/**
 * Code from a c-style string, setting the offset to zero
 * @param codeStr A string of code
 */
ScriptCode::ScriptCode(const char * codeStr) : m_code(codeStr), m_offset(0)
{
}

/**
 * Code from a QString, setting the offset to the given value
 * @param codeStr A string of code
 * @param offset An offset for the first line in a larger block of code
 */
ScriptCode::ScriptCode(const char * codeStr, const int offset)
  : m_code(codeStr), m_offset(offset)
{
}

/// Code from a QString with zero offset
ScriptCode::ScriptCode(const QString & codeStr)
  : m_code(codeStr.toStdString()), m_offset(0)
{
}

/// Code from a QString with a defined offset
ScriptCode::ScriptCode(const QString & codeStr, const int offset)
  : m_code(codeStr.toStdString()), m_offset(offset)
{
}
