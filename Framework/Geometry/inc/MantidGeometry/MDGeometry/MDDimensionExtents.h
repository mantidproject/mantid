// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/*
 * MDDimensionExtents.h
 *
 *  Created on: Jan 14, 2011
 *      Author: Janik Zikovsky
 */
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <limits>
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#endif

namespace Mantid {
namespace Geometry {

#pragma pack(push, 4) // Ensure the structure is no larger than it needs to

// the statement to exclude using macro min(a,b) in visual C++ uder win
#ifdef min
#undef min
#endif
// the statement to exclude using macro max(a,b) in visual C++ uder win
#ifdef max
#undef max
#endif

//===============================================================================================
/** Simple class that holds the extents (min/max)
 * of a given dimension in a MD workspace or MDBox
 */
template <typename T> class MDDimensionExtents {
public:
  /** Empty constructor - reset everything.
   *  */
  // ---- Public members ----------
  MDDimensionExtents() : min(1e30f), max(-1e30f), m_size(0.0f) {}
  T getSize() const { return m_size; }
  T getCentre() const { return static_cast<T>(0.5 * (max + min)); }
  bool outside(T x) const { return ((x < min) || (x >= max)); }
  bool isUndefined() const { return (min > max); }
  //
  std::string extentsStr() const {
    return (boost::lexical_cast<std::string>(min) + "-" + boost::lexical_cast<std::string>(max));
  }
  T getMin() const { return min; }
  T getMax() const { return max; }
  /// return the vertice in the grid, based on this extent's size
  T getGridVertex(const size_t ind) const { return min + m_size * static_cast<T>(ind); }

  void scaleExtents(double scaling, double offset) {
    min = static_cast<T>(min * scaling + offset);
    max = static_cast<T>(max * scaling + offset);
    m_size = static_cast<T>(m_size * scaling);
    if (max < min) {
      T tmp = max;
      max = min;
      min = tmp;
      m_size = std::fabs(m_size);
    }
  }
  // it looks like this loses accuracy?
  void expand(MDDimensionExtents &other) {
    double dMax = double(other.max);
    if (max > dMax)
      dMax = double(max);
    double dMin = double(other.min);
    if (min < dMin)
      dMin = double(min);

    other.max = static_cast<T>(dMax);
    other.min = static_cast<T>(dMin);
    other.m_size = static_cast<T>(dMax - dMin);
  }

  void setExtents(double dMin, double dMax) {
    min = static_cast<T>(dMin);
    max = static_cast<T>(dMax);
    m_size = static_cast<T>(dMax - dMin);
  }
  // private:
private:
  /// Extent: minimum value in that dimension
  T min;
  /// Extent: maximum value in that dimension
  T max;
  /// the box size; It is important to have box size defined from doubles to
  /// avoid accuracy loss
  /// when extracting two large float numbers min and max
  T m_size;
};

#pragma pack(pop) // Return to default packing size

} // namespace Geometry

} // namespace Mantid
