// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMPARSER_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMPARSER_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Poco {
namespace XML {
// Forward declaration
class Element;
} // namespace XML
} // namespace Poco

namespace Mantid {
namespace API {
/// Forward declaration
class CoordTransform;
} // namespace API

namespace DataObjects {

/** A parser for processing coordinate transform xml
 *
 * @author Owen Arnold
 * @date 22/july/2011
 */
class DLLExport CoordTransformAffineParser {
public:
  CoordTransformAffineParser();
  virtual Mantid::API::CoordTransform *
  createTransform(Poco::XML::Element *coordTransElement) const;
  virtual void setSuccessor(CoordTransformAffineParser *other);
  virtual ~CoordTransformAffineParser() = default;
  using SuccessorType_sptr =
      boost::shared_ptr<CoordTransformAffineParser>; ///< successor parser
                                                     ///< shared ptr typedef
protected:
  SuccessorType_sptr m_successor; ///< successor parser
private:
  /// Copy constructor
  CoordTransformAffineParser(const CoordTransformAffineParser &);
  /// Assignment operator
  CoordTransformAffineParser &operator=(const CoordTransformAffineParser &);
};
} // namespace DataObjects
} // namespace Mantid

#endif
