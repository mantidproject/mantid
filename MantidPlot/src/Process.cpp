// clang-format off
#include "MantidQtWidgets/Common/PythonThreading.h"
// clang-format on

#include "Process.h"

#include <iostream>
#include <stdexcept>
#include <QCoreApplication>

namespace {

class PyObjectNewReference {
public:
  PyObjectNewReference(PyObject *object) : m_object(object) {}
  ~PyObjectNewReference() { Py_XDECREF(m_object); }

  PyObjectNewReference(const PyObjectNewReference &) = delete;
  PyObjectNewReference &operator=(const PyObjectNewReference &) = delete;

  PyObjectNewReference(PyObjectNewReference &&) = default;
  PyObjectNewReference &operator=(PyObjectNewReference &&) = default;

  inline PyObject *ptr() const { return m_object; }

private:
  PyObject *m_object;
};

/**
 * @brief Retrieve a named attribute
 * @param source The source object
 * @param name The name of the attribute
 * @return The attribute
 * @throws std::runtime_error if an error occurs retrieving the attribute
 */
PyObjectNewReference attr(PyObject *source, const char *name) {
  PyObjectNewReference attr(PyObject_GetAttrString(source, name));
  if (attr.ptr()) {
    return attr;
  } else {
    PyErr_Clear();
    throw std::runtime_error(std::string("Process: No attribute ") + name +
                             " found");
  }
}

/**
 * @brief Call a named function with an check for errors
 * @param source The source object
 * @param name The name of the attribute to call
 * @return The return value of the function
 * @throws std::runtime_error if an error occurs retrieving the attribute
 */
PyObjectNewReference call(PyObject *source, const char *name) {
  auto result = PyObject_CallFunction(attr(source, name).ptr(), nullptr);
  if (result)
    return PyObjectNewReference(result);
  else {
    PyErr_Clear();
    throw std::runtime_error(std::string("Process: Error calling function ") +
                             name);
  }
}

/**
 * @return Return a pointer to the psutil module. A new reference is returned.
 */
PyObjectNewReference psutil() {
  if (auto process = PyImport_ImportModule("psutil")) {
    return PyObjectNewReference(process);
  } else {
    PyErr_Clear();
    throw std::runtime_error("Python module psutil cannot be imported.");
  }
}
} // namespace

namespace Process {

/**
 * @brief Return a list of process IDs for other instances of this process.
 * @return A list of other processes running. The PID for this process is
 * removed from the list. An empty list is returned
 * if no other processes are running.
 * @throws std::runtime_error if the PID list cannot be determined
 */
std::vector<int64_t> otherInstancePIDs() {
  ScopedPythonGIL lock;
  const int64_t ourPID(QCoreApplication::applicationPid());
  const PyObjectNewReference ourName(PyString_FromString(
      QCoreApplication::applicationName().toLatin1().data()));
  auto psutilModule(psutil());
  auto processIter(call(psutilModule.ptr(), "process_iter"));

  std::vector<int64_t> otherPIDs;
  PyObject *item(nullptr);
  while ((item = PyIter_Next(processIter.ptr()))) {
    auto name = call(item, "name");
    if (PyObject_RichCompareBool(name.ptr(), ourName.ptr(), Py_EQ)) {
      auto pid = PyLong_AsLong(attr(item, "pid").ptr());
      if (pid != ourPID) {
        otherPIDs.emplace_back(pid);
      }
    }
    Py_DECREF(item);
  }
  return otherPIDs;
}

} // namespace Process
