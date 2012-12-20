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

  typedef std::map<int, Qt::GlobalColor> ColourMapType;
  ColourMapType m_backgroundMap;
  ColourMapType m_foregroundMap;
  ColourMapType::iterator safeFetchPair(ColourMapType& map, const int index);
  ColourMapType::const_iterator safeFetchPair(const ColourMapType& map, const int index) const;
public:

    PeakPalette();
    PeakPalette(const PeakPalette& other);
    PeakPalette& operator=(const PeakPalette& other);
    Qt::GlobalColor foregroundIndexToColour(const int index) const;
    Qt::GlobalColor backgroundIndexToColour(const int index) const;
    void setForegroundColour(const int index, const Qt::GlobalColor);
    void setBackgroundColour(const int index, const Qt::GlobalColor);
    int paletteSize() const;
    ~PeakPalette();
};

} //namespace
}
#endif // MANTID_SLICEVIEWER_PEAKSPALETTE_H