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
    TS_ASSERT_EQUALS(Colour::Green, palette.foregroundIndexToColour(0));
    TS_ASSERT_EQUALS(Colour::DarkMagenta, palette.foregroundIndexToColour(1));
    TS_ASSERT_EQUALS(Colour::Cyan, palette.foregroundIndexToColour(2));
    TS_ASSERT_EQUALS(Colour::DarkGreen, palette.foregroundIndexToColour(3));
    TS_ASSERT_EQUALS(Colour::DarkCyan, palette.foregroundIndexToColour(4));
    TS_ASSERT_EQUALS(Colour::DarkYellow, palette.foregroundIndexToColour(5));
    TS_ASSERT_EQUALS(Colour::DarkRed, palette.foregroundIndexToColour(6));
    TS_ASSERT_EQUALS(Colour::Black, palette.foregroundIndexToColour(7));
    TS_ASSERT_EQUALS(Colour::White, palette.foregroundIndexToColour(8));
    TS_ASSERT_EQUALS(Colour::DarkGray, palette.foregroundIndexToColour(9));
  }  

  void test_default_backgroundIndexToColour()
  {
    PeakPalette palette;
    TS_ASSERT_EQUALS(Colour::Green, palette.backgroundIndexToColour(0));
    TS_ASSERT_EQUALS(Colour::DarkMagenta, palette.backgroundIndexToColour(1));
    TS_ASSERT_EQUALS(Colour::Cyan, palette.backgroundIndexToColour(2));
    TS_ASSERT_EQUALS(Colour::DarkGreen, palette.backgroundIndexToColour(3));
    TS_ASSERT_EQUALS(Colour::DarkCyan, palette.backgroundIndexToColour(4));
    TS_ASSERT_EQUALS(Colour::DarkYellow, palette.backgroundIndexToColour(5));
    TS_ASSERT_EQUALS(Colour::DarkRed, palette.backgroundIndexToColour(6));
    TS_ASSERT_EQUALS(Colour::Black, palette.backgroundIndexToColour(7));
    TS_ASSERT_EQUALS(Colour::White, palette.backgroundIndexToColour(8));
    TS_ASSERT_EQUALS(Colour::DarkGray, palette.backgroundIndexToColour(9));
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
    const Colour originalColour = palette.foregroundIndexToColour(indexToChange);
    const Colour requestColour = Colour::Black;

    palette.setForegroundColour(indexToChange, requestColour);

    const Colour finalColour = palette.foregroundIndexToColour(indexToChange);

    TSM_ASSERT_DIFFERS("\n\nForeground palette colour has not changed at requested index.\n", originalColour, finalColour);
    TSM_ASSERT_EQUALS("\n\nForeground palette colour has not changed to the requested colour.\n", requestColour, finalColour);

    const int expectedNumberOfEntries = 10;
    TSM_ASSERT_EQUALS("\n\nPalette should have a default and fixed size\n", expectedNumberOfEntries, palette.paletteSize());
  }

  void test_setBackgroundColour()
  {
    PeakPalette palette;
    const int indexToChange = 0;
    const Colour originalColour = palette.backgroundIndexToColour(indexToChange);
    const Colour requestColour = Colour::Black;

    palette.setForegroundColour(indexToChange, requestColour);

    const Colour finalColour = palette.foregroundIndexToColour(indexToChange);

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
    TSM_ASSERT_THROWS("\n\nIndex is > Max Index. Should throw\n.", palette.setForegroundColour(indexTooHigh, Colour::Red), std::out_of_range);
    TSM_ASSERT_THROWS("\n\nIndex is < Min Index. Should throw\n", palette.setForegroundColour(indexTooLow, Colour::Red), std::out_of_range);
  }

  void test_setBackgroundColour_throws_if_out_of_range()
  {
    const int indexTooHigh = 10;
    const int indexTooLow = -1;

    PeakPalette palette;
    TSM_ASSERT_THROWS("\n\nIndex is > Max Index. Should throw\n.", palette.setBackgroundColour(indexTooHigh, Colour::Red), std::out_of_range);
    TSM_ASSERT_THROWS("\n\nIndex is < Min Index. Should throw\n", palette.setBackgroundColour(indexTooLow, Colour::Red), std::out_of_range);
  }

  void testCopy()
  {
    // Create an original, and modify the palette a little, so we can be sure that the copy is a genuine copy of the current state.
    PeakPalette original;
    original.setForegroundColour(0, Colour::Red);
    original.setBackgroundColour(0, Colour::Blue);

    // Make a copy.
    PeakPalette copy(original);

    // Check the size.
    TSM_ASSERT_EQUALS("\n\nSize of the copy is not the same as the size of the original.\n", original.paletteSize(), copy.paletteSize());

    // Check all entries.
    for(int i = 0; i < original.paletteSize(); ++i)
    { 
      TSM_ASSERT_EQUALS("\n\n\Foreground colour different between orignial and copy", original.foregroundIndexToColour(i), copy.foregroundIndexToColour(i));
      TSM_ASSERT_EQUALS("\n\n\Background colour different between orignial and copy", original.backgroundIndexToColour(i), copy.backgroundIndexToColour(i));
    }

  }

  void testAssignment()
  {
    // Create an original, and modify the palette a little, so we can be sure that the copy is a genuine copy of the current state.
    PeakPalette A;
    A.setForegroundColour(0, Colour::Red);
    A.setBackgroundColour(0, Colour::Blue);

    // Make another.
    PeakPalette B;

    // Make A == B
    B = A;

    // Check the size.
    TSM_ASSERT_EQUALS("\n\nSize of the copy is not the same as the size of the original.\n", A.paletteSize(), B.paletteSize());

    // Check all entries.
    for(int i = 0; i < A.paletteSize(); ++i)
    { 
      TSM_ASSERT_EQUALS("\n\n\Foreground colour different between orignial and copy.\n", B.foregroundIndexToColour(i), A.foregroundIndexToColour(i));
      TSM_ASSERT_EQUALS("\n\n\Background colour different between orignial and copy.\n", B.backgroundIndexToColour(i), A.backgroundIndexToColour(i));
    }

    // Specifically check that B has taken A's values using a couple of test cases.
    TSM_ASSERT_EQUALS("\n\n\Assignment of foreground colours has not worked.\n", B.foregroundIndexToColour(0), Colour::Red);
    TSM_ASSERT_EQUALS("\n\n\Assignment of background colours has not worked.\n", B.backgroundIndexToColour(0), Colour::Blue);
  }

};

#endif
