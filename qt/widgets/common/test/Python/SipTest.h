// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTWIDGETS_SIPTEST_H
#define MANTIDQTWIDGETS_SIPTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

#include <QWidget>

class SipTest : public CxxTest::TestSuite {
public:
  static SipTest *createSuite() { return new SipTest; }
  static void destroySuite(SipTest *suite) { delete suite; }

  void setUp() override {
    Py_Initialize();
    PyEval_InitThreads();
    // Insert the directory of the properties file as a sitedir
    // to ensure the built copy of mantid gets picked up
    const MantidQt::Widgets::Common::Python::Object siteModule{
        MantidQt::Widgets::Common::Python::NewRef(
            PyImport_ImportModule("site"))};
    siteModule.attr("addsitedir")(
        Mantid::Kernel::ConfigService::Instance().getPropertiesDir());
    TS_ASSERT(Py_IsInitialized());
  }

  void tearDown() override {
    // Some test methods may leave the Python error handler with an error
    // set that confuse other tests when the executable is run as a whole
    // Clear the errors after each suite method is run
    PyErr_Clear();
    Py_Finalize();
  }

  // ----------------- success tests ---------------------
  void testExtractWithSipWrappedTypeSucceeds() {
    MantidQt::Widgets::Common::Python::Object qwidget{
        MantidQt::Widgets::Common::Python::NewRef(
            PyImport_ImportModule("qtpy.QtWidgets"))
            .attr("QWidget")()};
    QWidget *w{nullptr};
    TS_ASSERT_THROWS_NOTHING(
        w = MantidQt::Widgets::Common::Python::extract<QWidget>(qwidget));
    TS_ASSERT(w);
  }

  // ----------------- failure tests ---------------------

  void testExtractWithNonSipTypeThrowsException() {
    const MantidQt::Widgets::Common::Python::Object nonSipType{
        MantidQt::Widgets::Common::Python::NewRef(Py_BuildValue("(ii)", 1, 2))};
    struct Foo;
    TS_ASSERT_THROWS(
        MantidQt::Widgets::Common::Python::extract<Foo>(nonSipType),
        const std::runtime_error &);
  }
};

#endif // MANTIDQTWIDGETS_SIPTEST_H
