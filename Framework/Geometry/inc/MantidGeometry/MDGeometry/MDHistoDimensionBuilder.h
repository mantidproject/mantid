#ifndef MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_H_
#define MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_H_

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/UnitLabel.h"

#include <cmath>

namespace Mantid {
namespace Geometry {

/** MDHistoDimensionBuilder :
*
* A builder for the MDHistogram workspace. Required to construct a valid
*MDHistogram dimension, where values can
* cannot easily be brought togeter at once. Also allows potential construction
*values to be overwritten simply.
*
* @author Owen Arnold @ Tessella/ISIS
* @date 13/July/2011
*/

class MANTID_GEOMETRY_DLL MDHistoDimensionBuilder {
public:
  /**
   * Push the min/max values out by a defined amount. This is primarily used
   * for moving the dimensions boundaries so that an MDGridBox can
   * encompass all of the data and not chop of events if some events lie exactly
   * on a boundary
   * @param min A reference to the minimum value [InOut]
   * @param max A reference to the maximum value [InOut]
   */
  template <typename CoordT>
  static void resizeToFitMDBox(CoordT &min, CoordT &max) {
    // Always use minimum float value as DBL_EPS is always too small
    static constexpr CoordT twoEps = 2 * std::numeric_limits<float>::epsilon();
    if (std::fabs(min) > twoEps)
      min *= (1 - std::copysign(twoEps, min));
    else
      min -= twoEps;
    if (std::fabs(max) > twoEps)
      max *= (1 + std::copysign(twoEps, max));
    else
      max += twoEps;
  }

  MDHistoDimensionBuilder();
  void setName(std::string name);
  void setId(std::string id);
  void setUnits(const Kernel::UnitLabel &units);
  void setMin(double min);
  void setMax(double max);
  void setNumBins(size_t nbins);
  void setFrameName(std::string frameName);

  size_t getNumBins() const { return m_nbins; }
  MDHistoDimension *createRaw();
  IMDDimension_sptr create();

private:
  /// Cached name
  std::string m_name;
  /// Cached id
  std::string m_id;
  /// Cached units
  Kernel::UnitLabel m_units;
  /// Cached min
  double m_min;
  /// Cached max
  double m_max;
  /// Cached nbins
  size_t m_nbins;
  /// Flag indicating that min has been set.
  bool m_minSet;
  /// Flag indicating that max has been set.
  bool m_maxSet;
  /// Frame name
  std::string m_frameName;
};

/// Handy typedef for collection of builders.
using Vec_MDHistoDimensionBuilder = std::vector<MDHistoDimensionBuilder>;
}
}

#endif
