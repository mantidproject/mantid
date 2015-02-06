#ifndef MDGEOMETRY_XML_PARSER_H
#define MDGEOMETRY_XML_PARSER_H

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/DllConfig.h"
#include <string>

namespace Mantid {
namespace Geometry {

/** @class MDGeometryXMLParser

Handles the extraction of dimensions from a xml xml string to determine how
mappings have been formed.

@author Owen Arnold, Tessella Support Services plc
@date 17/05/2011

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL MDGeometryXMLParser {

private:
  bool m_executed;

  std::string m_rootNodeName;

  Mantid::Geometry::VecIMDDimension_sptr m_vecNonMappedDims;

  Mantid::Geometry::VecIMDDimension_sptr m_vecAllDims;

  Mantid::Geometry::IMDDimension_sptr m_xDimension;

  Mantid::Geometry::IMDDimension_sptr m_yDimension;

  Mantid::Geometry::IMDDimension_sptr m_zDimension;

  Mantid::Geometry::IMDDimension_sptr m_tDimension;

  void validate() const;

protected:
  std::string m_xmlToProcess;

  MDGeometryXMLParser();

public:
  explicit MDGeometryXMLParser(const std::string &xmlToProcess);

  virtual ~MDGeometryXMLParser();

  virtual void execute();

  Mantid::Geometry::IMDDimension_sptr getXDimension() const;

  Mantid::Geometry::IMDDimension_sptr getYDimension() const;

  Mantid::Geometry::IMDDimension_sptr getZDimension() const;

  Mantid::Geometry::IMDDimension_sptr getTDimension() const;

  Mantid::Geometry::VecIMDDimension_sptr getNonMappedDimensions() const;

  Mantid::Geometry::VecIMDDimension_sptr getNonIntegratedDimensions() const;

  Mantid::Geometry::VecIMDDimension_sptr getIntegratedDimensions() const;

  Mantid::Geometry::VecIMDDimension_sptr getAllDimensions() const;

  bool hasXDimension() const;

  bool hasYDimension() const;

  bool hasZDimension() const;

  bool hasTDimension() const;

  bool isXDimension(Mantid::Geometry::IMDDimension_sptr) const;

  bool isYDimension(Mantid::Geometry::IMDDimension_sptr) const;

  bool isZDimension(Mantid::Geometry::IMDDimension_sptr) const;

  bool isTDimension(Mantid::Geometry::IMDDimension_sptr) const;

  void SetRootNodeCheck(std::string elementName);

  MDGeometryXMLParser &operator=(const MDGeometryXMLParser &);

  MDGeometryXMLParser(const MDGeometryXMLParser &);
};
}
}

#endif
