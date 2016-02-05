#include "MantidQtSliceViewer/PeakPalette.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace MantidQt
{
  namespace SliceViewer
  {
    PeakPalette::PeakPalette()
    {
      m_foregroundMap = {{0, {"#bf7651"}}, {1, {"#bd97cb"}}, {2, {"#ceeeea"}},
                         {3, {"#da4a52"}}, {4, {"#9bc888"}}, {5, {"#ffe181"}},
                         {6, {"#e8b7c1"}}, {7, {"#f38235"}}, {8, {"#8390c6"}},
                         {9, {"#4ca0ac"}}};
      m_backgroundMap = {{0, {"#bf7651"}}, {1, {"#bd97cb"}}, {2, {"#ceeeea"}},
                         {3, {"#da4a52"}}, {4, {"#9bc888"}}, {5, {"#ffe181"}},
                         {6, {"#e8b7c1"}}, {7, {"#f38235"}}, {8, {"#8390c6"}},
                         {9, {"#4ca0ac"}}};
    }

    PeakPalette::PeakPalette(const PeakPalette& other) : m_backgroundMap(other.m_backgroundMap), m_foregroundMap(other.m_foregroundMap)
    {
    }

    PeakPalette& PeakPalette::operator=(const PeakPalette& other)
    {
      m_foregroundMap.clear();
      m_backgroundMap.clear();
      if(this != &other)
      {
        m_foregroundMap.insert(other.m_foregroundMap.begin(), other.m_foregroundMap.end());
        m_backgroundMap.insert(other.m_backgroundMap.begin(), other.m_backgroundMap.end());
      }
      return *this;
    }

    PeakPalette::~PeakPalette()
    {
    }

    void throwIndex(const int index)
    {
      std::stringstream stream;
      stream << "Index " << index << " is out of range";
      throw std::out_of_range(stream.str());
    }

    PeakPalette::ColourMapType::const_iterator PeakPalette::safeFetchPair(const PeakPalette::ColourMapType& map, const int index) const
    {
      auto it = map.find(index);
      if(it == map.end())
      {
        throwIndex(index);
      }
      return it;
    }


    PeakPalette::ColourMapType::iterator PeakPalette::safeFetchPair(PeakPalette::ColourMapType& map, const int index)
    {
      ColourMapType::iterator it = map.find(index);
      if(it == map.end())
      {
        throwIndex(index);
      }
      return it;
    }

    QColor PeakPalette::foregroundIndexToColour(const int index) const
    {
      auto it = safeFetchPair(m_foregroundMap, index);
      return it->second;
    }

    QColor PeakPalette::backgroundIndexToColour(const int index) const
    {
      auto it = safeFetchPair(m_backgroundMap, index);
      return it->second;
    }

    void PeakPalette::setForegroundColour(const int index, const QColor colour)
    {
      auto it = safeFetchPair(m_foregroundMap, index);
      // overwrite
      it->second = colour;
    }

    void PeakPalette::setBackgroundColour(const int index, const QColor colour)
    {
      auto it = safeFetchPair(m_backgroundMap, index);
      // owverwirte
      it->second = colour;
    }

    int PeakPalette::paletteSize() const
    {
      if(m_foregroundMap.size() != m_backgroundMap.size())
      {
        throw std::runtime_error("The PeakPalette size is not consistent");
      }
      return static_cast<int>(m_foregroundMap.size());
    }

    bool PeakPalette::operator==(const PeakPalette& other) const
    {
      bool areEqual = true;
      if(other.paletteSize() != this->paletteSize())
      {
        areEqual = false;
      }
      else
      {
        for(int i = 0; i < this->paletteSize(); ++i)
        {
          if(this->backgroundIndexToColour(i) != other.backgroundIndexToColour(i))
          {
            areEqual = false;
            break;
          }
          if(this->foregroundIndexToColour(i) != other.foregroundIndexToColour(i))
          {
            areEqual = false;
            break;
          }
        }
      }
      return areEqual;
    }

  }
}