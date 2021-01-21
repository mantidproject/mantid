// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include <memory>

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
  virtual Mantid::API::CoordTransform *createTransform(Poco::XML::Element *coordTransElement) const;
  virtual void setSuccessor(CoordTransformAffineParser *other);
  virtual ~CoordTransformAffineParser() = default;
  using SuccessorType_sptr = std::shared_ptr<CoordTransformAffineParser>; ///< successor parser
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
