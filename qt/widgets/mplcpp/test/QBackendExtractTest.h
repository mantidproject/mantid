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

class QBackendExtractTest : public CxxTest::TestSuite {
public:
  static QBackendExtractTest *createSuite() { return new QBackendExtractTest; }
  static void destroySuite(QBackendExtractTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testExtractWithSipWrappedTypeSucceeds() {
    MantidQt::Widgets::Common::Python::Object mplBackend{backendModule()};
    MantidQt::Widgets::Common::Python::Object fig{
        MantidQt::Widgets::Common::Python::NewRef(
            PyImport_ImportModule("matplotlib.figure"))
            .attr("Figure")()};
    MantidQt::Widgets::Common::Python::Object pyCanvas{
        mplBackend.attr("FigureCanvasQT")(fig)};
    QWidget *w{nullptr};
    TS_ASSERT_THROWS_NOTHING(
        w = MantidQt::Widgets::Common::Python::extract<QWidget>(pyCanvas));
    TS_ASSERT(w);
  }

  // ----------------- failure tests ---------------------

  void testExtractWithNonSipTypeThrowsException() {
    const MantidQt::Widgets::Common::Python::Object nonSipType{
        MantidQt::Widgets::Common::Python::NewRef(Py_BuildValue("(ii)", 1, 2))};
    struct Foo;
    TS_ASSERT_THROWS(
        MantidQt::Widgets::Common::Python::extract<Foo>(nonSipType),
        std::runtime_error);
  }
};

#endif // MPLCPP_QBACKENDEXTRACTTEST_H
