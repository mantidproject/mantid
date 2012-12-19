#include "MantidQtSliceViewer/PeakPalette.h"
#include <algorithm>
#include <sstream>

namespace MantidQt
{
  namespace SliceViewer
  {
    PeakPalette::PeakPalette()
    {
      int index = 0;
      m_foregroundMap.insert(std::make_pair(index++, Colour::Green));
      m_foregroundMap.insert(std::make_pair(index++, Colour::DarkMagenta));
      m_foregroundMap.insert(std::make_pair(index++, Colour::Cyan));
      m_foregroundMap.insert(std::make_pair(index++, Colour::DarkGreen));
      m_foregroundMap.insert(std::make_pair(index++, Colour::DarkCyan));
      m_foregroundMap.insert(std::make_pair(index++, Colour::DarkYellow));
      m_foregroundMap.insert(std::make_pair(index++, Colour::DarkRed));
      m_foregroundMap.insert(std::make_pair(index++, Colour::Black));
      m_foregroundMap.insert(std::make_pair(index++, Colour::White));
      m_foregroundMap.insert(std::make_pair(index++, Colour::DarkGray));
      m_backgroundMap = m_foregroundMap;
    }

    PeakPalette::PeakPalette(const PeakPalette& other) : m_foregroundMap(other.m_foregroundMap), m_backgroundMap(other.m_backgroundMap)
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

    Colour PeakPalette::foregroundIndexToColour(const int index) const
    {
      auto it = safeFetchPair(m_foregroundMap, index);
      return it->second;
    }

    Colour PeakPalette::backgroundIndexToColour(const int index) const
    {
      auto it = safeFetchPair(m_backgroundMap, index);
      return it->second;
    }

    void PeakPalette::setForegroundColour(const int index, const Colour colour)
    {
      auto it = safeFetchPair(m_foregroundMap, index);
      // overwrite
      it->second = colour;
    }

    void PeakPalette::setBackgroundColour(const int index, const Colour colour)
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
      return m_foregroundMap.size();
    }

  }
}