#ifndef MANTID_VATES_COMMON_H_
#define MANTID_VATES_COMMON_H_
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

class vtkFieldData;

namespace Mantid
{
namespace Geometry
{
//Forward dec
class IMDDimension;
}

namespace VATES
{
/// Vector of IMDDimension shared pointers.
typedef std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > DimensionVec;

/// IMDDimension as shared pointer.
typedef boost::shared_ptr<Mantid::Geometry::IMDDimension> Dimension_sptr;

/// IMDDimension as const shared pointer. Note that IMDDimension is pure virtual.
typedef boost::shared_ptr<const Mantid::Geometry::IMDDimension> Dimension_const_sptr;

std::string makeAxisTitle(Dimension_const_sptr dim);

/*
 * For legacy data sets we don't have unique latex labels, but they are rather
 * the same as the ascii string. There are several symbols we need to catch and
 * convert to latex. If you find other legacy symbols which need conversion,
 * then add them here.
 * @param input: the input string
 * @returns a converted string with the correct latex if possible, or
 * unconverted
 */
std::string convertAxesTitleToLatex(std::string toConvert);

void setAxisLabel(std::string metadataLabel,
                  std::string labelString,
                  vtkFieldData *fieldData);
}

}

#endif
