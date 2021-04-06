// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>

class ScriptEditor;

namespace MantidQt::Widgets::Common::Python {

/**
 * The CodeExecution class support execution of arbitrary Python code
 * with the option to install a trace handler to track lines executed and
 * tell an editor to mark them appropriately.
 */
class EXPORT_OPT_MANTIDQT_COMMON CodeExecution {
public:
  CodeExecution(ScriptEditor *editor);
  PyObject *execute(const QString &codeStr, const QString &filename, int flags, PyObject *globals,
                    int lineOffset) const;

private:
  ScriptEditor *m_editor{nullptr};
};

} // namespace MantidQt::Widgets::Common::Python
