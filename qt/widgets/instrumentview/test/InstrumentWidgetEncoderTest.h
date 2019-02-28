// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_INSTRUMENTWIDGETENCODERTEST_H_
#define MANTIDQT_WIDGETS_INSTRUMENTWIDGETENCODERTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h"

#include <QApplication>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

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
    PyEval_InitThreads();
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

static PythonInterpreter PYTHON_INTERPRETER;

/**
 * QApplication
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * QApplication object
 */
class QApplicationHolder : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    m_app = new QApplication(m_argc, m_argv);

    qRegisterMetaType<std::string>("StdString");
    qRegisterMetaType<Mantid::API::Workspace_sptr>("Workspace");

    return true;
  }

  bool tearDownWorld() override {
    delete m_app;
    return true;
  }

  int m_argc = 1;
  GNU_DIAG_OFF("pedantic")
  char *m_argv[1] = {"InstrumentWidgetTest"};
  GNU_DIAG_ON("pedantic")
  QApplication *m_app;
};

static QApplicationHolder MAIN_QAPPLICATION;

class InstrumentWidgetEncoderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentWidgetEncoderTest *createSuite() {
    return new InstrumentWidgetEncoderTest();
  }
  static void destroySuite(InstrumentWidgetEncoderTest *suite) { delete suite; }

  void setUp() override {
    FrameworkManager::Instance();
    auto alg =
        AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws");
    alg->execute();
    m_instrumentWidget = new InstrumentWidget(QString("ws"));
    m_encoder = new InstrumentWidgetEncoder();
  }

  void test_encode() {
    const auto result =
        m_encoder->encode(*m_instrumentWidget, QString(""), false);
    TS_ASSERT_EQUALS(result.size(), 7);
  }

  InstrumentWidget *m_instrumentWidget;
  InstrumentWidgetEncoder *m_encoder;
};

#endif /* MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODERTEST_H_ */