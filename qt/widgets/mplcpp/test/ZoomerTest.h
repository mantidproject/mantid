// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_ZOOMERTEST_H
#define MPLCPP_ZOOMERTEST_H

#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/Zoomer.h"

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::FigureCanvasQt;
using MantidQt::Widgets::MplCpp::Zoomer;
using namespace MantidQt::Widgets::Common;

class ZoomerTest : public CxxTest::TestSuite {
public:
  static ZoomerTest *createSuite() { return new ZoomerTest; }
  static void destroySuite(ZoomerTest *suite) { delete suite; }

public:
  // ----------------------------- success tests -------------------------------
  void testConstructionWithFigureCanvasSucceeds() {
    FigureCanvasQt canvas{111};
    TS_ASSERT_THROWS_NOTHING(Zoomer zoomer(&canvas));
  }

  void testDefaultHasZoomDisabled() {
    FigureCanvasQt canvas{111};
    Zoomer zoomer(&canvas);

    TS_ASSERT_EQUALS(false, zoomer.isZoomEnabled());
  }

  void testZoomOutDoesNotThrow() {
    FigureCanvasQt canvas{111};
    canvas.gca().plot({1, 2, 3, 4, 5}, {1, 2, 3, 4, 5});
    Zoomer zoomer(&canvas);
    zoomIn(&zoomer);

    TS_ASSERT_THROWS_NOTHING(zoomer.zoomOut());
    auto xlim(Python::Object(canvas.gca().pyobj().attr("get_xlim")()));
    // Do the axis limits get back to somewhere "close" to what is expected
    TS_ASSERT_DELTA(1.0, PyFloat_AsDouble(Python::Object(xlim[0]).ptr()), 0.25);
    TS_ASSERT_DELTA(5.0, PyFloat_AsDouble(Python::Object(xlim[1]).ptr()), 0.25);
  }

private:
  void zoomIn(Zoomer *zoomer) {
    zoomer->pyobj().attr("press_zoom")(createDummyMplMouseEvent(100, 100));
    // events myst be >=5 pixels apart to count
    zoomer->pyobj().attr("release_zoom")(createDummyMplMouseEvent(110, 110));
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

#endif // MPLCPP_ZOOMERTEST_H
