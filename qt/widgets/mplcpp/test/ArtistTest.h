#ifndef MPLCPP_ARTISTTEST_H
#define MPLCPP_ARTISTTEST_H

#include "MantidQtWidgets/MplCpp/Artist.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::Widgets::MplCpp;

class ArtistTest : public CxxTest::TestSuite {
public:
  static ArtistTest *createSuite() { return new ArtistTest; }
  static void destroySuite(ArtistTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testConstructWithArtistIsSuccessful() {
    Python::Object artistModule(
        Python::Handle<>(PyImport_ImportModule("matplotlib.artist")));
    Python::Object pyartist = artistModule.attr("Artist")();
    TS_ASSERT_THROWS_NOTHING(Artist drawer(pyartist));
  }

  void testArtistCallsRemoveOnPyObject() {
    Python::Object artistModule(
        Python::Handle<>(PyImport_ImportModule("matplotlib.text")));
    Artist label(artistModule.attr("Text")());
    TS_ASSERT_THROWS(label.remove(), Python::ErrorAlreadySet);
  }
  // ----------------- failure tests ---------------------

  void testConstructWithNonArtistThrowsInvalidArgument() {
    TS_ASSERT_THROWS(Artist(Python::Object()), std::invalid_argument);
  }
};

#endif // ARTISTTEST_H
