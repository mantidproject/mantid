#ifndef MPLCPP_SIPTEST_H
#define MPLCPP_SIPTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/MplCpp/BackendQt.h"
#include "MantidQtWidgets/MplCpp/Python/Sip.h"

#include <QWidget>

using MantidQt::Widgets::MplCpp::backendModule;
using namespace MantidQt::Widgets::MplCpp::Python;

class SipTest : public CxxTest::TestSuite {
public:
  static SipTest *createSuite() { return new SipTest; }
  static void destroySuite(SipTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testExtractWithSipWrappedTypeSucceeds() {
    auto mplBackend{backendModule()};
    auto fig{
        NewRef(PyImport_ImportModule("matplotlib.figure")).attr("Figure")()};
    auto pyCanvas{mplBackend.attr("FigureCanvasQT")(fig)};
    QWidget *w(nullptr);
    TS_ASSERT_THROWS_NOTHING(w = extract<QWidget>(pyCanvas));
    TS_ASSERT(w);
  }

  // ----------------- failure tests ---------------------

  void testExtractWithNonSipTypeThrowsException() {
    const auto nonSipType{NewRef(Py_BuildValue("(ii)", 1, 2))};
    struct Foo;
    TS_ASSERT_THROWS(extract<Foo>(nonSipType), std::runtime_error);
  }
};

#endif // MPLCPP_SIPTEST_H
