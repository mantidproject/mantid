// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PANPanpanZoomToolTest_H
#define MPLCPP_PANPanpanZoomToolTest_H

#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/PanZoomTool.h"

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::FigureCanvasQt;
using MantidQt::Widgets::MplCpp::PanZoomTool;
namespace Python = MantidQt::Widgets::MplCpp::Python;

class PanZoomToolTest : public CxxTest::TestSuite {
public:
  static PanZoomToolTest *createSuite() { return new PanZoomToolTest; }
  static void destroySuite(PanZoomToolTest *suite) { delete suite; }

public:
  // ----------------------------- success tests -------------------------------
  void testConstructionWithFigureCanvasSucceeds() {
    FigureCanvasQt canvas{111};
    TS_ASSERT_THROWS_NOTHING(PanZoomTool panZoomTool(&canvas));
  }

  void testDefaultHasPanAndZoomDisabled() {
    FigureCanvasQt canvas{111};
    PanZoomTool panZoomTool(&canvas);

    TS_ASSERT_EQUALS(false, panZoomTool.isZoomEnabled());
    TS_ASSERT_EQUALS(false, panZoomTool.isPanEnabled());
  }

  void testEnableZoomDisablesPan() {
    FigureCanvasQt canvas{111};
    PanZoomTool panZoomTool(&canvas);

    panZoomTool.enablePan(true);
    TS_ASSERT_EQUALS(true, panZoomTool.isPanEnabled());
    TS_ASSERT_EQUALS(false, panZoomTool.isZoomEnabled());

    panZoomTool.enableZoom(true);
    TS_ASSERT_EQUALS(false, panZoomTool.isPanEnabled());
    TS_ASSERT_EQUALS(true, panZoomTool.isZoomEnabled());
  }

  void testEnablePanDisablesZoom() {
    FigureCanvasQt canvas{111};
    PanZoomTool panZoomTool(&canvas);

    panZoomTool.enableZoom(true);
    TS_ASSERT_EQUALS(true, panZoomTool.isZoomEnabled());
    TS_ASSERT_EQUALS(false, panZoomTool.isPanEnabled());

    panZoomTool.enablePan(true);
    TS_ASSERT_EQUALS(false, panZoomTool.isZoomEnabled());
    TS_ASSERT_EQUALS(true, panZoomTool.isPanEnabled());
  }

  void testZoomOutDoesNotThrow() {
    FigureCanvasQt canvas{111};
    canvas.gca().plot({1, 2, 3, 4, 5}, {1, 2, 3, 4, 5});
    PanZoomTool panZoomTool(&canvas);
    zoomIn(&panZoomTool);

    TS_ASSERT_THROWS_NOTHING(panZoomTool.zoomOut());
    auto xlim(Python::Object(canvas.gca().pyobj().attr("get_xlim")()));
    // Do the axis limits get back to somewhere "close" to what is expected
    TS_ASSERT_DELTA(1.0, PyFloat_AsDouble(Python::Object(xlim[0]).ptr()), 0.25);
    TS_ASSERT_DELTA(5.0, PyFloat_AsDouble(Python::Object(xlim[1]).ptr()), 0.25);
  }

private:
  void zoomIn(PanZoomTool *panZoomTool) {
    panZoomTool->pyobj().attr("press_zoom")(createDummyMplMouseEvent(100, 100));
    // events myst be >=5 pixels apart to count
    panZoomTool->pyobj().attr("release_zoom")(
        createDummyMplMouseEvent(110, 110));
  }

  Python::Object createDummyMplMouseEvent(double xpos, double ypos) {
    try {
      auto mainModule = Python::NewRef(PyImport_ImportModule("__main__"));
      auto builtinsDict =
          Python::BorrowedRef(PyModule_GetDict(mainModule.ptr()));
      auto createMouseEventFnSrc =
          QString("def createDummyMouseEvent(xpos, ypos):\n"
                  "  class MouseEvent(object):\n"
                  "      x, y = xpos, ypos\n"
                  "      button = 1\n"
                  "      key = None\n"
                  "  return MouseEvent()\n");
      Python::Dict context;
      context.update(builtinsDict);
      Python::NewRef(PyRun_String(createMouseEventFnSrc.toLatin1().constData(),
                                  Py_file_input, context.ptr(), context.ptr()));
      return context["createDummyMouseEvent"](xpos, ypos);
    } catch (Python::ErrorAlreadySet &) {
      throw Mantid::PythonInterface::PythonException();
    }
  }
};

#endif // MPLCPP_PanpanZoomToolTest_H
