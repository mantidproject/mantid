#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"
#include <boost/format.hpp>
#include <cmath>
#include <stdexcept>

namespace MantidQt {
namespace SliceViewer {
/**
 * Default Constructor
 */
PeakBoundingBox::PeakBoundingBox()
    : m_left(0), m_right(0), m_top(0), m_bottom(0), m_slicePoint(0), m_front(0),
      m_back(0) {}

/**
 * Constructor
 * @param left: Box left
 * @param right : Box right
 * @param top : Box top
 * @param bottom : Box bottom
 * @param slicePoint : Slicing point.
 */
PeakBoundingBox::PeakBoundingBox(const Left &left, const Right &right,
                                 const Top &top, const Bottom &bottom,
                                 const SlicePoint &slicePoint)
    : m_left(left), m_right(right), m_top(top), m_bottom(bottom),
      m_slicePoint(slicePoint), m_front(Front(slicePoint())),
      m_back(Back(slicePoint())) {
  validateBoundaries();
}

/**
 * Constructor
 * @param left: Box left
 * @param right : Box right
 * @param top : Box top
 * @param bottom : Box bottom
 * @param slicePoint : Slicing point.
 * @param front : Box front
 * @param back : Box back
 */
PeakBoundingBox::PeakBoundingBox(const Left &left, const Right &right,
                                 const Top &top, const Bottom &bottom,
                                 const SlicePoint &slicePoint,
                                 const Front &front, const Back &back)
    : m_left(left), m_right(right), m_top(top), m_bottom(bottom),
      m_slicePoint(slicePoint), m_front(front), m_back(back) {
  validateBoundaries();
}

void PeakBoundingBox::validateBoundaries() {
  if (m_right() < m_left()) {
    throw std::invalid_argument("Right < Left");
  }
  if (m_top() < m_bottom()) {
    throw std::invalid_argument("Top < Bottom");
  }
  if (m_back() < m_front()) {
    throw std::invalid_argument("Back < Front");
  }
  if (m_back() < m_slicePoint()) {
    throw std::invalid_argument("Back < Slice Point");
  }
  if (m_slicePoint() < m_front()) {
    throw std::invalid_argument("Slice Point < Front");
  }
}

/// Destructor
PeakBoundingBox::~PeakBoundingBox() {}

/**
 * Copy constructor
 * @param other
 */
PeakBoundingBox::PeakBoundingBox(const PeakBoundingBox &other)
    : m_left(other.m_left), m_right(other.m_right), m_top(other.m_top),
      m_bottom(other.m_bottom), m_slicePoint(other.m_slicePoint),
      m_front(other.m_front), m_back(other.m_back) {}

/**
 * Assignment operator
 * @param other : Other box to assign from
 * @return : This object after assignment
 */
PeakBoundingBox &PeakBoundingBox::operator=(const PeakBoundingBox &other) {
  if (&other != this) {
    m_top = other.m_top;
    m_bottom = other.m_bottom;
    m_left = other.m_left;
    m_right = other.m_right;
    m_slicePoint = other.m_slicePoint;
    m_front = other.m_front;
    m_back = other.m_back;
  }
  return *this;
}

/**
 * Getter for left edge.
 * @return Left edge
 */
double PeakBoundingBox::left() const { return m_left(); }

/**
 * Getter for right edge.
 * @return Right edge
 */
double PeakBoundingBox::right() const { return m_right(); }

/**
 * Getter for top edge
 * @return Top edge
 */
double PeakBoundingBox::top() const { return m_top(); }

/**
 * Getter for bottom edge
 * @return bottom edge
 */
double PeakBoundingBox::bottom() const { return m_bottom(); }

/**
 * Getter for front edge
 * @return bottom edge
 */
double PeakBoundingBox::front() const { return m_front(); }

/**
 * Getter for front edge
 * @return bottom edge
 */
double PeakBoundingBox::back() const { return m_back(); }

/**
 * Getter for the slice point
 * @return The slice point
 */
double PeakBoundingBox::slicePoint() const { return m_slicePoint(); }

/**
 * Overloaded operator
 * @param other : other object to compare against
 * @return True if and only if the box and slice points are identical.
 */
bool PeakBoundingBox::operator==(const PeakBoundingBox &other) const {
  return this->m_left() == other.m_left() &&
         this->m_right() == other.m_right() &&
         this->m_bottom() == other.m_bottom() &&
         this->m_top() == other.m_top() &&
         this->m_slicePoint() == other.m_slicePoint() &&
         this->m_back() == other.m_back() && this->m_front() == other.m_front();
}

/**
 * Overloaded operator
 * @param other : other object to compare against
 * @return False if and only if both box and slice points are identical
 */
bool PeakBoundingBox::operator!=(const PeakBoundingBox &other) const {
  return !(*this == other);
}

/**
 * Make a new box using the SlicePosition and the sliceDelta to calculate a new
 * front and back edge.
 * @param sliceDelta : Thickness in z dimension
 * @return new Bounding box.
 */
PeakBoundingBox PeakBoundingBox::makeSliceBox(const double &sliceDelta) const {
  double halfWidth = std::abs(sliceDelta) / 2;
  return PeakBoundingBox(m_left, m_right, m_top, m_bottom, m_slicePoint,
                         Front(m_slicePoint() - halfWidth),
                         Back(m_back() + halfWidth));
}

/**
 * Export extents as as string of comma separated values.
 * @return vector of extents min, max in x, y, z
 */
std::vector<double> PeakBoundingBox::toExtents() const {
  std::vector<double> extents(6);
  extents[0] = m_left();
  extents[1] = m_right();
  extents[2] = m_bottom();
  extents[3] = m_top();
  extents[4] = m_front();
  extents[5] = m_back();
  return extents;
}

/**
 * Export extents as as string of comma separated values.
 * @return formatted comma separated string.
 */
std::string PeakBoundingBox::toExtentsString() const {
  using boost::format;
  auto extents = toExtents();
  return boost::str(format("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f") % extents[0] %
                    extents[1] % extents[2] % extents[3] % extents[4] %
                    extents[5]);
}

/**
 * Transform the box. Permanent  change the box left, right, top, bottom, front
 * and back according to the transform.
 * @param transform : Transform to use.
 */
void PeakBoundingBox::transformBox(
    Mantid::Geometry::PeakTransform_sptr transform) {
  using Mantid::Kernel::V3D;
  // Front bottom left
  V3D newBottomLeft =
      transform->transformBack(V3D(m_left(), m_bottom(), m_front()));
  // Back top right
  V3D newTopRight = transform->transformBack(V3D(m_right(), m_top(), m_back()));
  // SlicePoint
  V3D newSlicePoint = transform->transformBack(V3D(0, 0, m_slicePoint()));

  m_left = Left(newBottomLeft.X());
  m_bottom = Bottom(newBottomLeft.Y());
  m_right = Right(newTopRight.X());
  m_top = Top(newTopRight.Y());
  m_front = Front(newBottomLeft.Z());
  m_back = Back(newTopRight.Z());
  m_slicePoint = SlicePoint(newSlicePoint.Z());
}
} // namespace SliceViewer
} // namespace MantidQt
