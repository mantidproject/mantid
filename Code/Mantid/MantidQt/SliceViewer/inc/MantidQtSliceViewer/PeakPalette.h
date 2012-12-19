#ifndef MANTID_SLICEVIEWER_PEAKPALETTE_H
#define MANTID_SLICEVIEWER_PEAKPALETTE_H

#include "MantidKernel/System.h"
#include <map>

namespace MantidQt
{
namespace SliceViewer
{

  enum Colour
  {
    White=3,
    Black=2,
    Red=7,
    DarkRed=13,
    Green=8,
    DarkGreen=14,
    Blue=9,
    DarkBlue=15,
    Cyan=10,
    DarkCyan=16,
    Magenta=11,
    DarkMagenta=17,
    Yellow=12,
    DarkYellow=18,
    Gray=5,
    DarkGray=4,
    LightGray=6,
    Transparent=19,
    Color0=0,
    Color1=1
  };

class DLLExport PeakPalette
{
private:

  typedef std::map<int, Colour> ColourMapType;
  ColourMapType m_backgroundMap;
  ColourMapType m_foregroundMap;
  ColourMapType::iterator safeFetchPair(ColourMapType& map, const int index);
  ColourMapType::const_iterator safeFetchPair(const ColourMapType& map, const int index) const;
public:

    PeakPalette();
    PeakPalette(const PeakPalette& other);
    PeakPalette& operator=(const PeakPalette& other);
    Colour foregroundIndexToColour(const int index) const;
    Colour backgroundIndexToColour(const int index) const;
    void setForegroundColour(const int index, const Colour);
    void setBackgroundColour(const int index, const Colour);
    int paletteSize() const;
    ~PeakPalette();
};

} //namespace
}
#endif // MANTID_SLICEVIEWER_PEAKSPALETTE_H