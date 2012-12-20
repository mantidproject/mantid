#ifndef SLICE_VIEWER_PEAKPALETTE_TEST_H_
#define SLICE_VIEWER_PEAKPALETTE_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PeakPalette.h"
#include <sstream>

using namespace MantidQt::SliceViewer;

class PeakPaletteTest : public CxxTest::TestSuite
{

public:

  void test_paletteSize()
  {
    PeakPalette palette;
    const int expectedNumberOfEntries = 10;
    TSM_ASSERT_EQUALS("\n\nPalette should have a default and fixed size\n", expectedNumberOfEntries, palette.paletteSize());
  }

  void test_default_foregroundIndexToColour()
  {
    PeakPalette palette;
    TS_ASSERT_EQUALS(Qt::green, palette.foregroundIndexToColour(0));
    TS_ASSERT_EQUALS(Qt::darkMagenta, palette.foregroundIndexToColour(1));
    TS_ASSERT_EQUALS(Qt::cyan, palette.foregroundIndexToColour(2));
    TS_ASSERT_EQUALS(Qt::darkGreen, palette.foregroundIndexToColour(3));
    TS_ASSERT_EQUALS(Qt::darkCyan, palette.foregroundIndexToColour(4));
    TS_ASSERT_EQUALS(Qt::darkYellow, palette.foregroundIndexToColour(5));
    TS_ASSERT_EQUALS(Qt::darkRed, palette.foregroundIndexToColour(6));
    TS_ASSERT_EQUALS(Qt::black, palette.foregroundIndexToColour(7));
    TS_ASSERT_EQUALS(Qt::white, palette.foregroundIndexToColour(8));
    TS_ASSERT_EQUALS(Qt::darkGray, palette.foregroundIndexToColour(9));
  }  

  void test_default_backgroundIndexToColour()
  {
    PeakPalette palette;
    TS_ASSERT_EQUALS(Qt::green, palette.backgroundIndexToColour(0));
    TS_ASSERT_EQUALS(Qt::darkMagenta, palette.backgroundIndexToColour(1));
    TS_ASSERT_EQUALS(Qt::cyan, palette.backgroundIndexToColour(2));
    TS_ASSERT_EQUALS(Qt::darkGreen, palette.backgroundIndexToColour(3));
    TS_ASSERT_EQUALS(Qt::darkCyan, palette.backgroundIndexToColour(4));
    TS_ASSERT_EQUALS(Qt::darkYellow, palette.backgroundIndexToColour(5));
    TS_ASSERT_EQUALS(Qt::darkRed, palette.backgroundIndexToColour(6));
    TS_ASSERT_EQUALS(Qt::black, palette.backgroundIndexToColour(7));
    TS_ASSERT_EQUALS(Qt::white, palette.backgroundIndexToColour(8));
    TS_ASSERT_EQUALS(Qt::darkGray, palette.backgroundIndexToColour(9));
  }

  void test_foregroundIndexToColour_throws_if_out_of_range()
  {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette palette;
    TSM_ASSERT_THROWS("\n\nIndex > Max Index, should throw.\n", palette.foregroundIndexToColour(indexTooHigh), std::out_of_range);
    TSM_ASSERT_THROWS("\n\nIndex < Max Index, should throw.\n", palette.foregroundIndexToColour(indexTooLow), std::out_of_range);
  }

  void test_backgroundIndexToColour_throws_if_out_of_range()
  {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette palette;
    TSM_ASSERT_THROWS("\n\nIndex > Max Index, should throw.\n", palette.backgroundIndexToColour(indexTooHigh), std::out_of_range);
    TSM_ASSERT_THROWS("\n\nIndex < Max Index, should throw.\n", palette.backgroundIndexToColour(indexTooLow), std::out_of_range);
  }

  void test_setForgroundColour()
  {
    PeakPalette palette;
    const int indexToChange = 0;
    const Qt::GlobalColor originalColour = palette.foregroundIndexToColour(indexToChange);
    const Qt::GlobalColor requestColour = Qt::black;

    palette.setForegroundColour(indexToChange, requestColour);

    const Qt::GlobalColor finalColour = palette.foregroundIndexToColour(indexToChange);

    TSM_ASSERT_DIFFERS("\n\nForeground palette colour has not changed at requested index.\n", originalColour, finalColour);
    TSM_ASSERT_EQUALS("\n\nForeground palette colour has not changed to the requested colour.\n", requestColour, finalColour);

    const int expectedNumberOfEntries = 10;
    TSM_ASSERT_EQUALS("\n\nPalette should have a default and fixed size\n", expectedNumberOfEntries, palette.paletteSize());
  }

  void test_setBackgroundColour()
  {
    PeakPalette palette;
    const int indexToChange = 0;
    const Qt::GlobalColor originalColour = palette.backgroundIndexToColour(indexToChange);
    const Qt::GlobalColor requestColour = Qt::black;

    palette.setForegroundColour(indexToChange, requestColour);

    const Qt::GlobalColor finalColour = palette.foregroundIndexToColour(indexToChange);

    TSM_ASSERT_DIFFERS("\n\nBackground palette colour has not changed at requested index.\n", originalColour, finalColour);
    TSM_ASSERT_EQUALS("\n\nBackground palette colour has not changed to the requested colour.\n", requestColour, finalColour);

    const int expectedNumberOfEntries = 10;
    TSM_ASSERT_EQUALS("\n\nPalette should have a default and fixed size\n", expectedNumberOfEntries, palette.paletteSize());
  }

  void test_setForegroundColour_throws_if_out_of_range()
  {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette palette;
    TSM_ASSERT_THROWS("\n\nIndex is > Max Index. Should throw\n.", palette.setForegroundColour(indexTooHigh, Qt::red), std::out_of_range);
    TSM_ASSERT_THROWS("\n\nIndex is < Min Index. Should throw\n", palette.setForegroundColour(indexTooLow, Qt::red), std::out_of_range);
  }

  void test_setBackgroundColour_throws_if_out_of_range()
  {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette palette;
    TSM_ASSERT_THROWS("\n\nIndex is > Max Index. Should throw\n.", palette.setBackgroundColour(indexTooHigh, Qt::red), std::out_of_range);
    TSM_ASSERT_THROWS("\n\nIndex is < Min Index. Should throw\n", palette.setBackgroundColour(indexTooLow, Qt::red), std::out_of_range);
  }

  void testCopy()
  {
    // Create an original, and modify the palette a little, so we can be sure that the copy is a genuine copy of the current state.
    PeakPalette original;
    original.setForegroundColour(0, Qt::red);
    original.setBackgroundColour(0, Qt::blue);

    // Make a copy.
    PeakPalette copy(original);

    // Check the size.
    TSM_ASSERT_EQUALS("\n\nSize of the copy is not the same as the size of the original.\n", original.paletteSize(), copy.paletteSize());

    // Check all entries.
    for(int i = 0; i < original.paletteSize(); ++i)
    { 
      TSM_ASSERT_EQUALS("\n\nForeground colour different between orignial and copy", original.foregroundIndexToColour(i), copy.foregroundIndexToColour(i));
      TSM_ASSERT_EQUALS("\n\nBackground colour different between orignial and copy", original.backgroundIndexToColour(i), copy.backgroundIndexToColour(i));
    }

  }

  void testAssignment()
  {
    // Create an original, and modify the palette a little, so we can be sure that the copy is a genuine copy of the current state.
    PeakPalette A;
    A.setForegroundColour(0, Qt::red);
    A.setBackgroundColour(0, Qt::blue);

    // Make another.
    PeakPalette B;

    // Make A == B
    B = A;

    // Check the size.
    TSM_ASSERT_EQUALS("\n\nSize of the copy is not the same as the size of the original.\n", A.paletteSize(), B.paletteSize());

    // Check all entries.
    for(int i = 0; i < A.paletteSize(); ++i)
    { 
      TSM_ASSERT_EQUALS("\n\nForeground colour different between orignial and copy.\n", B.foregroundIndexToColour(i), A.foregroundIndexToColour(i));
      TSM_ASSERT_EQUALS("\n\nBackground colour different between orignial and copy.\n", B.backgroundIndexToColour(i), A.backgroundIndexToColour(i));
    }

    // Specifically check that B has taken A's values using a couple of test cases.
    TSM_ASSERT_EQUALS("\n\nAssignment of foreground colours has not worked.\n", B.foregroundIndexToColour(0), Qt::red);
    TSM_ASSERT_EQUALS("\n\nAssignment of background colours has not worked.\n", B.backgroundIndexToColour(0), Qt::blue);
  }

};

#endif
