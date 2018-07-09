#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMDISTANCEPARSER_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMDISTANCEPARSER_H_

#include "MantidDataObjects/CoordTransformAffineParser.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace DataObjects {
/// Forward declaration
class CoordTransformDistance;

/** A parser for processing coordinate transform xml
 *
 * @author Owen Arnold
 * @date 25/july/2011
 */
class DLLExport CoordTransformDistanceParser
    : public CoordTransformAffineParser {
public:
  CoordTransformDistanceParser();
  Mantid::API::CoordTransform *
  createTransform(Poco::XML::Element *coordTransElement) const override;

private:
  CoordTransformDistanceParser(const CoordTransformDistanceParser &);
  CoordTransformDistanceParser &operator=(const CoordTransformDistanceParser &);
};
} // namespace DataObjects
} // namespace Mantid

#endif
