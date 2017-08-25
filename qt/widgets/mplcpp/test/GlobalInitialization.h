#ifndef PYTHONINITIALIZATION_H
#define PYTHONINITIALIZATION_H

#include <QApplication>

#include "cxxtest/GlobalFixture.h"
#include "MantidQtWidgets/Common/PythonThreading.h"

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
    return Py_IsInitialized();
  }

  bool tearDownWorld() override {
    Py_Finalize();
    return true;
  }
};

/**
 * QApplicationInit
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

#endif // PYTHONINITIALIZATION_H
