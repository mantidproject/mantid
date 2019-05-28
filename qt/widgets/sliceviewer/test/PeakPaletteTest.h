// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SLICE_VIEWER_PEAKPALETTE_TEST_H_
#define SLICE_VIEWER_PEAKPALETTE_TEST_H_

#include "MantidQtWidgets/SliceViewer/PeakPalette.h"
#include <cxxtest/TestSuite.h>
#include <sstream>

using namespace MantidQt::SliceViewer;

class PeakPaletteTest : public CxxTest::TestSuite {

public:
  void test_paletteSize() {
    PeakPalette<QColor> palette;
    const int expectedNumberOfEntries = 10;
    TSM_ASSERT_EQUALS("\n\nPalette should have a default and fixed size\n",
                      expectedNumberOfEntries, palette.paletteSize());
  }

  void test_default_foreground_colours_unique() {
    PeakPalette<QColor> palette;
    for (int i = 0; i < palette.paletteSize() - 1; ++i) {
      TS_ASSERT_DIFFERS(palette.foregroundIndexToColour(i),
                        palette.foregroundIndexToColour(i + 1));
    }
  }

  void test_default_background_colours_unique() {
    PeakPalette<QColor> palette;
    for (int i = 0; i < palette.paletteSize() - 1; ++i) {
      TS_ASSERT_DIFFERS(palette.backgroundIndexToColour(i),
                        palette.backgroundIndexToColour(i + 1));
    }
  }

  void test_foregroundIndexToColour_throws_if_out_of_range() {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette<QColor> palette;
    TSM_ASSERT_THROWS("\n\nIndex > Max Index, should throw.\n",
                      palette.foregroundIndexToColour(indexTooHigh),
                      const std::out_of_range &);
    TSM_ASSERT_THROWS("\n\nIndex < Max Index, should throw.\n",
                      palette.foregroundIndexToColour(indexTooLow),
                      const std::out_of_range &);
  }

  void test_backgroundIndexToColour_throws_if_out_of_range() {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette<QColor> palette;
    TSM_ASSERT_THROWS("\n\nIndex > Max Index, should throw.\n",
                      palette.backgroundIndexToColour(indexTooHigh),
                      const std::out_of_range &);
    TSM_ASSERT_THROWS("\n\nIndex < Max Index, should throw.\n",
                      palette.backgroundIndexToColour(indexTooLow),
                      const std::out_of_range &);
  }

  void test_setForgroundColour() {
    PeakPalette<QColor> palette;
    const int indexToChange = 0;
    const QColor originalColour =
        palette.foregroundIndexToColour(indexToChange);
    const QColor requestColour(Qt::black);

    palette.setForegroundColour(indexToChange, requestColour);

    const QColor finalColour = palette.foregroundIndexToColour(indexToChange);

    TSM_ASSERT_DIFFERS(
        "\n\nForeground palette colour has not changed at requested index.\n",
        originalColour, finalColour);
    TSM_ASSERT_EQUALS("\n\nForeground palette colour has not changed to the "
                      "requested colour.\n",
                      requestColour, finalColour);

    const int expectedNumberOfEntries = 10;
    TSM_ASSERT_EQUALS("\n\nPalette should have a default and fixed size\n",
                      expectedNumberOfEntries, palette.paletteSize());
  }

  void test_setBackgroundColour() {
    PeakPalette<QColor> palette;
    const int indexToChange = 0;
    const QColor originalColour =
        palette.backgroundIndexToColour(indexToChange);
    const QColor requestColour = Qt::black;

    palette.setForegroundColour(indexToChange, requestColour);

    const QColor finalColour = palette.foregroundIndexToColour(indexToChange);

    TSM_ASSERT_DIFFERS(
        "\n\nBackground palette colour has not changed at requested index.\n",
        originalColour, finalColour);
    TSM_ASSERT_EQUALS("\n\nBackground palette colour has not changed to the "
                      "requested colour.\n",
                      requestColour, finalColour);

    const int expectedNumberOfEntries = 10;
    TSM_ASSERT_EQUALS("\n\nPalette should have a default and fixed size\n",
                      expectedNumberOfEntries, palette.paletteSize());
  }

  void test_setForegroundColour_throws_if_out_of_range() {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette<QColor> palette;
    TSM_ASSERT_THROWS("\n\nIndex is > Max Index. Should throw\n.",
                      palette.setForegroundColour(indexTooHigh, Qt::red),
                      const std::out_of_range &);
    TSM_ASSERT_THROWS("\n\nIndex is < Min Index. Should throw\n",
                      palette.setForegroundColour(indexTooLow, Qt::red),
                      const std::out_of_range &);
  }

  void test_setBackgroundColour_throws_if_out_of_range() {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette<QColor> palette;
    TSM_ASSERT_THROWS("\n\nIndex is > Max Index. Should throw\n.",
                      palette.setBackgroundColour(indexTooHigh, Qt::red),
                      const std::out_of_range &);
    TSM_ASSERT_THROWS("\n\nIndex is < Min Index. Should throw\n",
                      palette.setBackgroundColour(indexTooLow, Qt::red),
                      const std::out_of_range &);
  }

  void testCopy() {
    // Create an original, and modify the palette a little, so we can be sure
    // that the copy is a genuine copy of the current state.
    PeakPalette<QColor> original;
    original.setForegroundColour(0, Qt::red);
    original.setBackgroundColour(0, Qt::blue);

    // Make a copy.
    PeakPalette<QColor> copy(original);

    // Check the size.
    TSM_ASSERT_EQUALS(
        "\n\nSize of the copy is not the same as the size of the original.\n",
        original.paletteSize(), copy.paletteSize());

    // Check all entries.
    for (int i = 0; i < original.paletteSize(); ++i) {
      TSM_ASSERT_EQUALS(
          "\n\nForeground colour different between orignial and copy",
          original.foregroundIndexToColour(i), copy.foregroundIndexToColour(i));
      TSM_ASSERT_EQUALS(
          "\n\nBackground colour different between orignial and copy",
          original.backgroundIndexToColour(i), copy.backgroundIndexToColour(i));
    }
  }

  void testAssignment() {
    // Create an original, and modify the palette a little, so we can be sure
    // that the copy is a genuine copy of the current state.
    PeakPalette<QColor> A;
    A.setForegroundColour(0, Qt::red);
    A.setBackgroundColour(0, Qt::blue);

    // Make another.
    PeakPalette<QColor> B;

    // Make A == B
    B = A;

    // Check the size.
    TSM_ASSERT_EQUALS(
        "\n\nSize of the copy is not the same as the size of the original.\n",
        A.paletteSize(), B.paletteSize());

    // Check all entries.
    for (int i = 0; i < A.paletteSize(); ++i) {
      TSM_ASSERT_EQUALS(
          "\n\nForeground colour different between orignial and copy.\n",
          B.foregroundIndexToColour(i), A.foregroundIndexToColour(i));
      TSM_ASSERT_EQUALS(
          "\n\nBackground colour different between orignial and copy.\n",
          B.backgroundIndexToColour(i), A.backgroundIndexToColour(i));
    }

    TS_ASSERT_EQUALS(A, B);

    // Specifically check that B has taken A's values using a couple of test
    // cases.
    TSM_ASSERT_EQUALS("\n\nAssignment of foreground colours has not worked.\n",
                      B.foregroundIndexToColour(0), Qt::red);
    TSM_ASSERT_EQUALS("\n\nAssignment of background colours has not worked.\n",
                      B.backgroundIndexToColour(0), Qt::blue);
  }

  void test_are_equal() {
    PeakPalette<QColor> A;
    PeakPalette<QColor> B;

    TS_ASSERT_EQUALS(A, B);
  }

  void test_are_not_equal_after_changing_a_foreground_colour() {
    PeakPalette<QColor> A;
    PeakPalette<QColor> B;

    const int modifyIndex = 0;
    QColor originalColourAtIndex = A.backgroundIndexToColour(modifyIndex);

    A.setForegroundColour(modifyIndex, Qt::blue);
    B.setForegroundColour(modifyIndex, Qt::red);

    TSM_ASSERT_DIFFERS(
        "Foreground colours are not equal, these palettes should not be equal.",
        A, B);

    // For completeness, reset and check they are the same.
    A.setForegroundColour(modifyIndex, originalColourAtIndex);
    B.setForegroundColour(modifyIndex, originalColourAtIndex);
    TS_ASSERT_EQUALS(A, B);
  }

  void test_not_not_equal_after_changing_a_backgroundcolour() {
    PeakPalette<QColor> A;
    PeakPalette<QColor> B;

    const int modifyIndex = 0;
    QColor originalColourAtIndex = A.backgroundIndexToColour(modifyIndex);

    A.setBackgroundColour(modifyIndex, Qt::blue);
    B.setBackgroundColour(modifyIndex, Qt::red);

    TSM_ASSERT_DIFFERS(
        "Background colours are not equal, these palettes should not be equal.",
        A, B);

    // For completeness, reset and check they are the same.
    A.setBackgroundColour(modifyIndex, originalColourAtIndex);
    B.setBackgroundColour(modifyIndex, originalColourAtIndex);
    TS_ASSERT_EQUALS(A, B);
  }

  // ----- Tests for PeakViewColor
  void
  test_that_peak_view_color_specialization_produces_output_with_three_colors() {
    // Arrange
    PeakPalette<PeakViewColor> palette;

    const int index = 2;

    // Act + Assert
    PeakViewColor foregroundColor;
    PeakViewColor backgroundColor;

    TSM_ASSERT_THROWS_NOTHING("Should happily create the foreground color",
                              foregroundColor =
                                  palette.foregroundIndexToColour(index));
    TSM_ASSERT_THROWS_NOTHING("Should happily create the background color",
                              backgroundColor =
                                  palette.backgroundIndexToColour(index));
  }
};

#endif
