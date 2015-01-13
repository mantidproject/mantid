#ifndef MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_H_
#define MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_H_

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

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
  MDHistoDimensionBuilder();
  ~MDHistoDimensionBuilder();
  void setName(std::string name);
  void setId(std::string id);
  void setUnits(std::string units);
  void setMin(double min);
  void setMax(double max);
  void setNumBins(size_t nbins);

  size_t getNumBins() const { return m_nbins; }
  MDHistoDimension *createRaw();
  IMDDimension_sptr create();
  MDHistoDimensionBuilder(const MDHistoDimensionBuilder &);
  MDHistoDimensionBuilder &operator=(const MDHistoDimensionBuilder &);

private:
  /// Cached name
  std::string m_name;
  /// Cached id
  std::string m_id;
  /// Cached units
  std::string m_units;
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
};

/// Handy typedef for collection of builders.
typedef std::vector<MDHistoDimensionBuilder> Vec_MDHistoDimensionBuilder;
}
}

#endif