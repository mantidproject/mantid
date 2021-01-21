// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Python/CodeExecution.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include "MantidQtWidgets/Common/ScriptEditor.h"
#include <QHash>
#include <QString>
#include <frameobject.h>

using Mantid::PythonInterface::GlobalInterpreterLock;

namespace {

struct ScriptEditorDetails {
  ScriptEditor *editor;
  int lineOffset;
};

// Map co_filename objects from PyCodeObject to an editor object
QHash<PyObject *, ScriptEditorDetails> EDITOR_LOOKUP;

/**
 * A callback, set using PyEval_SetTrace, that is called by Python
 * to allow inspection into the current execution frame. It is currently
 * used to emit the line number of the frame that is being executed.
 * @param obj :: A reference to the object passed as the second argument
 * of PyEval_SetTrace. Assumed nullptr and unused
 * @param frame :: A reference to the current frame object
 * @param event :: An integer defining the event type, see
 * http://docs.python.org/c-api/init.html#profiling-and-tracing
 * @param arg :: Meaning varies depending on event type, see
 * http://docs.python.org/c-api/init.html#profiling-and-tracing
 */
int traceLineNumber(PyObject *obj, PyFrameObject *frame, int event, PyObject *arg) {
  Q_UNUSED(obj);
  Q_UNUSED(arg);
  if (event != PyTrace_LINE)
    return 0;
  auto iter = EDITOR_LOOKUP.constFind(frame->f_code->co_filename);
  if (iter != EDITOR_LOOKUP.constEnd()) {
    const auto &details = iter.value();
    int lineLoc = frame->f_lineno + details.lineOffset;
    details.editor->updateProgressMarkerFromThread(lineLoc, false);
  }
  return 0;
}
} // namespace

namespace MantidQt::Widgets::Common::Python {

/**
 * Construct a LineTrackingExecutor for a given editor
 * @param editor A pointer to an editor. Can be nullptr. Disables progress
 * tracking.
 */
CodeExecution::CodeExecution(ScriptEditor *editor) : m_editor(editor) {}

/**
 * Execute the code string from the given filename and return the result
 * @param codeStr A string containing the source code
 * @param filename A string containing the filename of the source code
 * @param flags An OR-ed combination of compiler flags
 * @param globals A dictionary containing the current globals mapping
 * @param lineOffset The number of lines offset to apply to the marker
 */
PyObject *CodeExecution::execute(const QString &codeStr, const QString &filename, int flags, PyObject *globals,
                                 int lineOffset) const {
  GlobalInterpreterLock gil;
  PyCompilerFlags compileFlags;
  compileFlags.cf_flags = flags;
  auto compiledCode =
      Py_CompileStringFlags(codeStr.toUtf8().constData(), filename.toUtf8().constData(), Py_file_input, &compileFlags);
  if (!compiledCode) {
    return nullptr;
  }

  if (!m_editor) {
    return PyEval_EvalCode(CODE_OBJECT(compiledCode), globals, globals);
  }

  ScriptEditorDetails editor_details{m_editor, lineOffset};
  const auto coFileObject = ((PyCodeObject *)compiledCode)->co_filename;
  const auto posIter = EDITOR_LOOKUP.insert(coFileObject, editor_details);
  PyEval_SetTrace((Py_tracefunc)&traceLineNumber, nullptr);
  const auto result = PyEval_EvalCode(CODE_OBJECT(compiledCode), globals, globals);
  PyEval_SetTrace(nullptr, nullptr);
  EDITOR_LOOKUP.erase(posIter);
  return result;

} // namespace MantidQt::Widgets::Common::Python

} // namespace MantidQt::Widgets::Common::Python
