#ifndef MANTID_GEOMETRY_MDHISTODIMENSION_H_
#define MANTID_GEOMETRY_MDHISTODIMENSION_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MDUnitFactory.h"
#include "MantidKernel/VMD.h"

namespace Mantid {
namespace Geometry {

/** MDHistoDimension :
 *
 * A very simple implementation of a IMDDimension, describing the
 * limits and number of bins in a dimension.
 *
 * @author Janik Zikovsky
 * @date 2011-03-24 14:16:39.231608
 */
class MANTID_GEOMETRY_DLL MDHistoDimension : public IMDDimension {
public:
  /** Constructor for simple MDHistoDimension
   * @param name :: full name of the axis
   * @param ID :: identifier string
   * @param frame :: MDFrame
   * @param min :: minimum extent
   * @param max :: maximum extent
   * @param numBins :: number of bins (evenly spaced)
   */
  MDHistoDimension(std::string name, std::string ID, const MDFrame &frame,
                   coord_t min, coord_t max, size_t numBins)
      : m_name(name), m_dimensionId(ID), m_frame(frame.clone()), m_min(min),
        m_max(max), m_numBins(numBins),
        m_binWidth((max - min) / static_cast<coord_t>(numBins)) {
    if (max < min) {
      throw std::invalid_argument("Error making MDHistoDimension. Cannot have "
                                  "dimension with min > max");
    }
  }

  /** copy constructor
   * @param other :: other IMDDimension
   */
  MDHistoDimension(const IMDDimension *other)
      : m_name(other->getName()), m_dimensionId(other->getDimensionId()),
        m_frame(other->getMDFrame().clone()), m_min(other->getMinimum()),
        m_max(other->getMaximum()), m_numBins(other->getNBins()),
        m_binWidth(other->getBinWidth()) {}

  /// Return the name of the dimension as can be displayed along the axis
  std::string getName() const override { return m_name; }

  /// Return the md frame
  const MDFrame &getMDFrame() const override { return *m_frame; }

  /// Return the units of the dimension as a string
  const Kernel::UnitLabel getUnits() const override {
    return m_frame->getUnitLabel();
  }

  /// Returns the unit
  const Kernel::MDUnit &getMDUnits() const override {
    return m_frame->getMDUnit();
  }

  /** Short name which identify the dimension among other dimension.
   * A dimension can be usually found by its ID and various
   * various method exist to manipulate set of dimensions by their names.
   */
  const std::string &getDimensionId() const override { return m_dimensionId; }

  /// Returns the maximum extent of this dimension
  coord_t getMaximum() const override { return m_max; }

  /// Returns the minimum extent of this dimension
  coord_t getMinimum() const override { return m_min; }

  /// number of bins dimension have (an integrated has one). A axis directed
  /// along dimension would have getNBins+1 axis points.
  size_t getNBins() const override { return m_numBins; }

  /// number of bin boundaries (axis points)
  size_t getNBoundaries() const override { return m_numBins + 1; }

  /// Dimensions must be xml serializable.
  std::string toXMLString() const override;

  ///  Get coordinate for index;
  coord_t getX(size_t index) const override {
    return static_cast<coord_t>(index) * m_binWidth + m_min;
  }

  /// Return the width of one bin.
  coord_t getBinWidth() const override { return m_binWidth; }

  /** Change the extents and number of bins
   * @param nBins :: number of bins
   * @param min :: extents minimum
   * @param max :: extents maximum
   */
  void setRange(size_t nBins, coord_t min, coord_t max) override {
    if (max < min) {
      throw std::invalid_argument("Error making MDHistoDimension. Cannot have "
                                  "dimension with min > max");
    }
    m_min = min;
    m_max = max;
    m_numBins = nBins;
    m_binWidth = (m_max - m_min) / static_cast<coord_t>(m_numBins);
  }

  /**
   * Set the MDFrame. This method was added in order to set the correct
   * MDFrame information on workspaces which are loaded from legacy files.
   * This is currently being used by SetMDFrame. Except for legacy corrections
   * you should not have to use of this method. If you think you do, it
   * is advisible to consolut with other Mantid team members before using it.
   * @param frame:: a reference to a new MDFrame
   */
  void setMDFrame(const MDFrame &frame) { m_frame.reset(frame.clone()); }

private:
  /// Name
  std::string m_name;

  /// ID string
  std::string m_dimensionId;

  /// Multidimensional frame
  Geometry::MDFrame_uptr m_frame;

  /// Extent of dimension
  coord_t m_min;

  /// Extent of dimension
  coord_t m_max;

  /// Number of bins
  size_t m_numBins;

  /// Calculated bin size
  coord_t m_binWidth;
};

/// Shared pointer to a MDHistoDimension
using MDHistoDimension_sptr = boost::shared_ptr<MDHistoDimension>;

/// Shared pointer to a const MDHistoDimension
using MDHistoDimension_const_sptr = boost::shared_ptr<const MDHistoDimension>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MDHISTODIMENSION_H_ */
