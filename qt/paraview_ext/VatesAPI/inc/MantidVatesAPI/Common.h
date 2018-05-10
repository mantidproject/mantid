#ifndef MANTID_VATES_COMMON_H_
#define MANTID_VATES_COMMON_H_
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

class vtkFieldData;

namespace Mantid {
namespace Geometry {
// Forward dec
class IMDDimension;
} // namespace Geometry

namespace VATES {
/// Vector of IMDDimension shared pointers.
using DimensionVec =
    std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension>>;

/// IMDDimension as shared pointer.
using Dimension_sptr = boost::shared_ptr<Mantid::Geometry::IMDDimension>;

/// IMDDimension as const shared pointer. Note that IMDDimension is pure
/// virtual.
using Dimension_const_sptr =
    boost::shared_ptr<const Mantid::Geometry::IMDDimension>;

std::string makeAxisTitle(const Mantid::Geometry::IMDDimension &dim);

/*
 * For legacy data sets we don't have unique latex labels, but they are rather
 * the same as the ascii string. There are several symbols we need to catch and
 * convert to latex. If you find other legacy symbols which need conversion,
 * then add them here.
 * @param input: the input string
 * @returns a converted string with the correct latex if possible, or
 * unconverted
 */
std::string convertAxesTitleToLatex(const std::string &toConvert);

void setAxisLabel(const std::string &metadataLabel,
                  const std::string &labelString, vtkFieldData *fieldData);
} // namespace VATES
} // namespace Mantid

#endif
