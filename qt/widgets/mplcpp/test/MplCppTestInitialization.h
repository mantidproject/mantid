#ifndef MPLCPPTESTGLOBALINITIALIZATION_H
#define MPLCPPTESTGLOBALINITIALIZATION_H

#include "cxxtest/GlobalFixture.h"

#include <QApplication>
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/VersionCompat.h"

/**
 * PythonInterpreter
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * Python
 */
class PythonInterpreter : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    Py_Initialize();
    Mantid::PythonInterface::importNumpy();
    return Py_IsInitialized();
  }

  bool tearDown() override {
    // Some test methods may leave the Python error handler with an error
    // set that confuse other tests when the executable is run as a whole
    // Clear the errors after each suite method is run
    PyErr_Clear();
    return CxxTest::GlobalFixture::tearDown();
  }

  bool tearDownWorld() override {
    Py_Finalize();
    return true;
  }
};

/**
 * QApplication
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * QApplication object
 */
class QApplicationHolder : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    int argc(0);
    char **argv = {};
    m_app = new QApplication(argc, argv);
    return true;
  }

  bool tearDownWorld() override {
    delete m_app;
    return true;
  }

private:
  QApplication *m_app;
};

//------------------------------------------------------------------------------
// Static definitions
//
// We rely on cxxtest only including this file once so that the following
// statements do not cause multiple-definition errors.
//------------------------------------------------------------------------------
static PythonInterpreter PYTHON_INTERPRETER;
static QApplicationHolder MAIN_QAPPLICATION;

#endif // MPLCPPTESTGLOBALINITIALIZATION_H
