// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/MplCpp/Artist.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::Widgets::MplCpp;
using namespace MantidQt::Widgets::Common;

class ArtistTest : public CxxTest::TestSuite {
public:
  static ArtistTest *createSuite() { return new ArtistTest; }
  static void destroySuite(ArtistTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testConstructWithArtistIsSuccessful() {
    auto artistModule(Python::NewRef(PyImport_ImportModule("matplotlib.artist")));
    Python::Object pyartist = artistModule.attr("Artist")();
    TS_ASSERT_THROWS_NOTHING(Artist drawer(pyartist));
  }

  void testSetCallsArtistSetForSingleProperty() {
    auto textModule(Python::NewRef(PyImport_ImportModule("matplotlib.text")));
    Artist label(textModule.attr("Text")());
    label.set("color", "r");

    TS_ASSERT_EQUALS("r", label.pyobj().attr("get_color")());
  }

  void testSetCallsArtistSetForDictProperties() {
    auto textModule(Python::NewRef(PyImport_ImportModule("matplotlib.text")));
    Artist label(textModule.attr("Text")());
    Python::Dict kwargs;
    kwargs["color"] = "r";
    kwargs["alpha"] = 0.5;
    label.set(kwargs);

    TS_ASSERT_EQUALS("r", label.pyobj().attr("get_color")());
    TS_ASSERT_EQUALS(0.5, label.pyobj().attr("get_alpha")());
  }

  void testArtistCallsRemoveOnPyObject() {
    using Mantid::PythonInterface::PythonException;
    auto textModule(Python::NewRef(PyImport_ImportModule("matplotlib.text")));
    Artist label(textModule.attr("Text")());
    TS_ASSERT_THROWS(label.remove(), const PythonException &);
  }
  // ----------------- failure tests ---------------------

  void testConstructWithNonArtistThrowsInvalidArgument() {
    Python::Object none;
    TS_ASSERT_THROWS(Artist artist(none), const std::invalid_argument &);
  }
};
