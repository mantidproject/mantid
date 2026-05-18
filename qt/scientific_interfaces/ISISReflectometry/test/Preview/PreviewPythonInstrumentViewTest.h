// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include "PreviewPythonInstrumentView.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using Mantid::PythonInterface::GlobalInterpreterLock;

class PreviewPythonInstrumentViewTest : public CxxTest::TestSuite {
public:
  static PreviewPythonInstrumentViewTest *createSuite() { return new PreviewPythonInstrumentViewTest(); }
  static void destroySuite(PreviewPythonInstrumentViewTest *suite) { delete suite; }

  PreviewPythonInstrumentViewTest() : m_view() {}

  void test_python_object_has_update_workspace_method() { assertPythonViewHasMethod("update_workspace"); }

  void test_python_object_has_reset_method() { assertPythonViewHasMethod("reset"); }

  void test_python_object_has_plot_method() { assertPythonViewHasMethod("plot"); }

  void test_python_object_has_set_zoom_mode_method() { assertPythonViewHasMethod("set_zoom_mode"); }

  void test_python_object_has_set_select_rect_mode_method() { assertPythonViewHasMethod("set_select_rect_mode"); }

  void test_python_object_has_selected_detector_ids_method() { assertPythonViewHasMethod("selected_detector_ids"); }

  void test_python_object_has_view_attribute() { assertPythonViewHasMethod("view"); }

  void test_get_selected_detector_ids_returns_empty_initially() { TS_ASSERT(m_view.getSelectedDetectorIDs().empty()); }

  void test_reset_inst_view_does_not_throw() { TS_ASSERT_THROWS_NOTHING(m_view.resetInstView()); }

  void test_plot_inst_view_does_not_throw_with_no_workspace() { TS_ASSERT_THROWS_NOTHING(m_view.plotInstView()); }

private:
  PreviewPythonInstrumentView m_view;

  void assertPythonViewHasMethod(const char *attr) {
    GlobalInterpreterLock lock;
    TS_ASSERT(PyObject_HasAttrString(m_view.pyobj().ptr(), attr));
  }
};
