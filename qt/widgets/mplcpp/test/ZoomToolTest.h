// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_ZOOMTOOLTEST_H
#define MPLCPP_ZOOMTOOLTEST_H

#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/ZoomTool.h"

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::FigureCanvasQt;
using MantidQt::Widgets::MplCpp::ZoomTool;
using namespace MantidQt::Widgets::Common;
namespace Python = MantidQt::Widgets::MplCpp::Python;

class ZoomToolTest : public CxxTest::TestSuite {
public:
  static ZoomToolTest *createSuite() { return new ZoomToolTest; }
  static void destroySuite(ZoomToolTest *suite) { delete suite; }

public:
  // ----------------------------- success tests -------------------------------
  void testConstructionWithFigureCanvasSucceeds() {
    FigureCanvasQt canvas{111};
    TS_ASSERT_THROWS_NOTHING(ZoomTool zoomTool(&canvas));
  }

  void testDefaultHasZoomDisabled() {
    FigureCanvasQt canvas{111};
    ZoomTool zoomTool(&canvas);

    TS_ASSERT_EQUALS(false, zoomTool.isZoomEnabled());
  }

  void testZoomOutDoesNotThrow() {
    FigureCanvasQt canvas{111};
    canvas.gca().plot({1, 2, 3, 4, 5}, {1, 2, 3, 4, 5});
    ZoomTool zoomTool(&canvas);
    zoomIn(&zoomTool);

    TS_ASSERT_THROWS_NOTHING(zoomTool.zoomOut());
    auto xlim(Python::Object(canvas.gca().pyobj().attr("get_xlim")()));
    // Do the axis limits get back to somewhere "close" to what is expected
    TS_ASSERT_DELTA(1.0, PyFloat_AsDouble(Python::Object(xlim[0]).ptr()), 0.25);
    TS_ASSERT_DELTA(5.0, PyFloat_AsDouble(Python::Object(xlim[1]).ptr()), 0.25);
  }

private:
  void zoomIn(ZoomTool *zoomTool) {
    zoomTool->pyobj().attr("press_zoom")(createDummyMplMouseEvent(100, 100));
    // events myst be >=5 pixels apart to count
    zoomTool->pyobj().attr("release_zoom")(createDummyMplMouseEvent(110, 110));
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

#endif // MPLCPP_ZOOMTOOLTEST_H
