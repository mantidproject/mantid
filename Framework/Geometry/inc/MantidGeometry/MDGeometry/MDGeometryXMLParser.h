// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDGEOMETRY_XML_PARSER_H
#define MDGEOMETRY_XML_PARSER_H

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <string>

namespace Mantid {
namespace Geometry {

/** @class MDGeometryXMLParser

Handles the extraction of dimensions from a xml xml string to determine how
mappings have been formed.

@author Owen Arnold, Tessella Support Services plc
@date 17/05/2011
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

  virtual ~MDGeometryXMLParser() = default;

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
} // namespace Geometry
} // namespace Mantid

#endif
