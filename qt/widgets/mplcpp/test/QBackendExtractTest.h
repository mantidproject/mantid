// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_QBACKENDEXTRACTTEST_H
#define MPLCPP_QBACKENDEXTRACTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/Common/Python/Sip.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"

#include <QWidget>

using MantidQt::Widgets::MplCpp::backendModule;
using namespace MantidQt::Widgets::Common;

class QBackendExtractTest : public CxxTest::TestSuite {
public:
  static QBackendExtractTest *createSuite() { return new QBackendExtractTest; }
  static void destroySuite(QBackendExtractTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testExtractWithSipWrappedTypeSucceeds() {
    Python::Object mplBackend{backendModule()};
    Python::Object fig{
        Python::NewRef(PyImport_ImportModule("matplotlib.figure"))
            .attr("Figure")()};
    Python::Object pyCanvas{mplBackend.attr("FigureCanvasQT")(fig)};
    QWidget *w{nullptr};
    TS_ASSERT_THROWS_NOTHING(w = Python::extract<QWidget>(pyCanvas));
    TS_ASSERT(w);
  }

  // ----------------- failure tests ---------------------

  void testExtractWithNonSipTypeThrowsException() {
    const Python::Object nonSipType{
        Python::NewRef(Py_BuildValue("(ii)", 1, 2))};
    struct Foo;
    TS_ASSERT_THROWS(Python::extract<Foo>(nonSipType), const std::runtime_error &);
  }
};

#endif // MPLCPP_QBACKENDEXTRACTTEST_H
