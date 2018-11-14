// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_
#define MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_

#include "DllOption.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace SliceViewer {
/**
DoubleParam
IntToType Parameter Type. Simple mechanism for ensuring type
safety when working with so many arguments of the same core type in
PeakBoundingBox.
*/
template <int I> class EXPORT_OPT_MANTIDQT_SLICEVIEWER DoubleParam {
public:
  explicit DoubleParam(const double &val) : value(val) {}
  DoubleParam(const DoubleParam<I> &other) : value(other.value) {}
  DoubleParam<I> &operator=(const DoubleParam<I> &other) {
    value = other.value;
    return *this;
  }
  double operator()() const { return value; }

private:
  double value;
  enum { typeValue = I };
};

using Left = DoubleParam<0>;
using Right = DoubleParam<1>;
using Top = DoubleParam<2>;
using Bottom = DoubleParam<3>;
using SlicePoint = DoubleParam<4>;
using Front = DoubleParam<5>;
using Back = DoubleParam<6>;

/** A bounding box for a peak. Allows the SliceViewer to zoom to that region.

@date 2013-01-09
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakBoundingBox {
private:
  /// Left edge
  Left m_left;
  /// Right edge
  Right m_right;
  /// Top edge
  Top m_top;
  /// Bottom edge.
  Bottom m_bottom;
  /// Slice parallel to projection (z) position
  SlicePoint m_slicePoint;
  /// Front edge
  Front m_front;
  /// Back edge
  Back m_back;
  /// Check boundaries
  void validateBoundaries();

public:
  /// Default constructor
  PeakBoundingBox();
  /// Constructor
  PeakBoundingBox(const Left &left, const Right &right, const Top &top,
                  const Bottom &bottom, const SlicePoint &slicePoint);
  /// Constructor
  PeakBoundingBox(const Left &left, const Right &right, const Top &top,
                  const Bottom &bottom, const SlicePoint &slicePoint,
                  const Front &front, const Back &back);
  /// Destructor
  ~PeakBoundingBox();
  /// Copy constructor
  PeakBoundingBox(const PeakBoundingBox &other);
  /// Assignment
  PeakBoundingBox &operator=(const PeakBoundingBox &other);
  /// Equals
  bool operator==(const PeakBoundingBox &other) const;
  /// Not equals
  bool operator!=(const PeakBoundingBox &other) const;
  /// Get the box left edge
  double left() const;
  /// Get the box right edge
  double right() const;
  /// Get the box top edge
  double top() const;
  /// Get the box bottom edge
  double bottom() const;
  /// Get the slice point
  double slicePoint() const;
  /// Get the back edge
  double front() const;
  /// Get the front edge
  double back() const;
  /// Serialize as a vector of extents.
  std::vector<double> toExtents() const;
  /// Serialize as set of comma separated values
  std::string toExtentsString() const;
  /// Transform the box.
  void transformBox(Mantid::Geometry::PeakTransform_sptr transform);
  /// Make a new box based on the slice
  PeakBoundingBox makeSliceBox(const double &sliceDelta) const;
};
} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_ */
