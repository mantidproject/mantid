// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAKPALETTE_H
#define MANTID_SLICEVIEWER_PEAKPALETTE_H

#include "MantidKernel/System.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"
#include <QColor>
#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>

namespace MantidQt {
namespace SliceViewer {

template <typename C> class DLLExport PeakPalette {
public:
  PeakPalette();
  PeakPalette(const PeakPalette &other);
  PeakPalette &operator=(const PeakPalette &other);
  C foregroundIndexToColour(const int index) const;
  C backgroundIndexToColour(const int index) const;
  void setForegroundColour(const int index, const C /*colour*/);
  void setBackgroundColour(const int index, const C /*colour*/);
  int paletteSize() const;
  bool operator==(const PeakPalette &other) const;
  ~PeakPalette();

private:
  using ColourMapType = std::map<int, C>;
  ColourMapType m_backgroundMap;
  ColourMapType m_foregroundMap;
  typename ColourMapType::iterator safeFetchPair(ColourMapType &map,
                                                 const int index) {
    typename ColourMapType::iterator it = map.find(index);
    if (it == map.end()) {
      std::stringstream stream;
      stream << "Index " << index << " is out of range";
      throw std::out_of_range(stream.str());
    }
    return it;
  }

  typename ColourMapType::const_iterator safeFetchPair(const ColourMapType &map,
                                                       const int index) const {
    auto it = map.find(index);
    if (it == map.end()) {
      std::stringstream stream;
      stream << "Index " << index << " is out of range";
      throw std::out_of_range(stream.str());
    }
    return it;
  }
};

template <typename C> PeakPalette<C>::PeakPalette() {}

template <typename C>
PeakPalette<C>::PeakPalette(const PeakPalette &other)
    : m_backgroundMap(other.m_backgroundMap),
      m_foregroundMap(other.m_foregroundMap) {}

template <typename C>
PeakPalette<C> &PeakPalette<C>::operator=(const PeakPalette<C> &other) {
  m_foregroundMap.clear();
  m_backgroundMap.clear();
  if (this != &other) {
    m_foregroundMap.insert(other.m_foregroundMap.begin(),
                           other.m_foregroundMap.end());
    m_backgroundMap.insert(other.m_backgroundMap.begin(),
                           other.m_backgroundMap.end());
  }
  return *this;
}

template <typename C> PeakPalette<C>::~PeakPalette() {}

template <typename C>
C PeakPalette<C>::foregroundIndexToColour(const int index) const {
  auto it = safeFetchPair(m_foregroundMap, index);
  return it->second;
}

template <typename C>
C PeakPalette<C>::backgroundIndexToColour(const int index) const {
  auto it = safeFetchPair(m_backgroundMap, index);
  return it->second;
}

template <typename C>
void PeakPalette<C>::setForegroundColour(const int index, const C colour) {
  auto it = safeFetchPair(m_foregroundMap, index);
  // overwrite
  it->second = colour;
}

template <typename C>
void PeakPalette<C>::setBackgroundColour(const int index, const C colour) {
  auto it = safeFetchPair(m_backgroundMap, index);
  // owverwirte
  it->second = colour;
}

template <typename C> int PeakPalette<C>::paletteSize() const {
  if (m_foregroundMap.size() != m_backgroundMap.size()) {
    throw std::runtime_error("The PeakPalette size is not consistent");
  }
  return static_cast<int>(m_foregroundMap.size());
}

template <typename C>
bool PeakPalette<C>::operator==(const PeakPalette &other) const {
  bool areEqual = true;
  if (other.paletteSize() != this->paletteSize()) {
    areEqual = false;
  } else {
    for (int i = 0; i < this->paletteSize(); ++i) {
      if (this->backgroundIndexToColour(i) !=
          other.backgroundIndexToColour(i)) {
        areEqual = false;
        break;
      }
      if (this->foregroundIndexToColour(i) !=
          other.foregroundIndexToColour(i)) {
        areEqual = false;
        break;
      }
    }
  }
  return areEqual;
}

// Forward declaration for template specialization
template <> DLLExport PeakPalette<QColor>::PeakPalette();
template <> DLLExport PeakPalette<PeakViewColor>::PeakPalette();

} // namespace SliceViewer
} // namespace MantidQt
#endif // MANTID_SLICEVIEWER_PEAKSPALETTE_H