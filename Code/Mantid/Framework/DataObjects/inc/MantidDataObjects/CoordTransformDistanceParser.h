#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMDISTANCEPARSER_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMDISTANCEPARSER_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include "MantidMDEvents/CoordTransformAffineParser.h"

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
  virtual Mantid::API::CoordTransform *
  createTransform(Poco::XML::Element *coordTransElement) const;
  virtual ~CoordTransformDistanceParser();

private:
  CoordTransformDistanceParser(const CoordTransformDistanceParser &);
  CoordTransformDistanceParser &operator=(const CoordTransformDistanceParser &);
};
}
}

#endif
