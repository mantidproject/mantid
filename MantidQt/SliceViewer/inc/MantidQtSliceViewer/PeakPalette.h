#ifndef MANTID_SLICEVIEWER_PEAKPALETTE_H
#define MANTID_SLICEVIEWER_PEAKPALETTE_H

#include "MantidKernel/System.h"
#include <map>
#include <QColor>

namespace MantidQt
{
namespace SliceViewer
{

class DLLExport PeakPalette
{
private:

  typedef std::map<int, QColor> ColourMapType;
  ColourMapType m_backgroundMap;
  ColourMapType m_foregroundMap;
  ColourMapType::iterator safeFetchPair(ColourMapType& map, const int index);
  ColourMapType::const_iterator safeFetchPair(const ColourMapType& map, const int index) const;
public:

    PeakPalette();
    PeakPalette(const PeakPalette& other);
    PeakPalette& operator=(const PeakPalette& other);
    QColor foregroundIndexToColour(const int index) const;
    QColor backgroundIndexToColour(const int index) const;
    void setForegroundColour(const int index, const QColor);
    void setBackgroundColour(const int index, const QColor);
    int paletteSize() const;
    bool operator==(const PeakPalette& other) const;
    ~PeakPalette();
};

} //namespace
}
#endif // MANTID_SLICEVIEWER_PEAKSPALETTE_H