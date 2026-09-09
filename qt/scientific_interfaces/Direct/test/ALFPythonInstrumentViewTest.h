// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFPythonInstrumentView.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapPython.h"

#include <cxxtest/TestSuite.h>

#include <QWidget>

using namespace MantidQt::CustomInterfaces;
using Mantid::PythonInterface::GlobalInterpreterLock;

class ALFPythonInstrumentViewTest : public CxxTest::TestSuite {
public:
  static ALFPythonInstrumentViewTest *createSuite() { return new ALFPythonInstrumentViewTest(); }
  static void destroySuite(ALFPythonInstrumentViewTest *suite) { delete suite; }

  ALFPythonInstrumentViewTest() : m_view() {}

  void test_construction_creates_the_python_presenter() {
    GlobalInterpreterLock lock;
    TS_ASSERT(m_view.pyobj().ptr() != nullptr);
    TS_ASSERT(m_view.pyobj().ptr() != Py_None);
  }

  void test_python_object_has_update_view_method() { assertPythonPresenterHasAttr("update_view"); }

  void test_python_object_has_selected_detector_indices_by_tube_method() {
    assertPythonPresenterHasAttr("selected_detector_indices_by_tube");
  }

  void test_python_object_has_view_attribute() { assertPythonPresenterHasAttr("_view"); }

  void test_get_instrument_view_returns_a_widget() { TS_ASSERT(m_view.getInstrumentView() != nullptr); }

private:
  ALFPythonInstrumentView m_view;

  void assertPythonPresenterHasAttr(const char *attr) {
    GlobalInterpreterLock lock;
    TS_ASSERT(PyObject_HasAttrString(m_view.pyobj().ptr(), attr));
  }
};
