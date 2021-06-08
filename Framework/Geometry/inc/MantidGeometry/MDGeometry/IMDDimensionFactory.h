// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Geometry {
/** Creates IMDDimension objects based on input XML.
 *
 *
 */

MANTID_GEOMETRY_DLL IMDDimension_sptr createDimension(const std::string &dimensionXMLString);
MANTID_GEOMETRY_DLL IMDDimension_sptr createDimension(const Poco::XML::Element &dimensionXML);
MANTID_GEOMETRY_DLL IMDDimension_sptr createDimension(const std::string &dimensionXMLString, int nBins, coord_t min,
                                                      coord_t max);
} // namespace Geometry
} // namespace Mantid
