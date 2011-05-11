#ifndef MDGEOMETRYXMLBUILDER_H_
#define MDGEOMETRYXMLBUILDER_H_

#include <vector>
#include <string>
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Mantid
{
namespace Geometry
{

/**
 @class  MDGeometryBuliderXML
 @brief Computes Boolean algebra for simplification
 @author S. Ansell
 @date August 2005
 @version 1.0

 Handles the generation of well formed description of a geometry based on input IMDDimensions.
 Outputs xml.

 Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>

 */
class DLLExport MDGeometryBuilderXML
{

public:

  /// Constructor
  MDGeometryBuilderXML();

  /// Destructor
  ~MDGeometryBuilderXML();

  /// Add a dimension that is neither considered x, y, z or t.
  bool addOrdinaryDimension(IMDDimension_const_sptr dimension);

  /// Add x dimension.
  bool addXDimension(IMDDimension_const_sptr dimension);

  /// Add y dimension.
  bool addYDimension(IMDDimension_const_sptr dimension);

  /// Add z dimension.
  bool addZDimension(IMDDimension_const_sptr dimension);

  /// Add t dimension.
  bool addTDimension(IMDDimension_const_sptr dimension);


  /// Create the xml.
  std::string create();

private:

  typedef std::vector<IMDDimension_const_sptr> DimensionContainerType;

  DimensionContainerType m_vecDimensions;

  IMDDimension_const_sptr m_spXDimension;

  IMDDimension_const_sptr m_spYDimension;

  IMDDimension_const_sptr m_spZDimension;

  IMDDimension_const_sptr m_spTDimension;

  MDGeometryBuilderXML(const MDGeometryBuilderXML&);

  MDGeometryBuilderXML& operator=(const MDGeometryBuilderXML&);

  /// Determine whetether a valid x dimension has been provided.
  bool hasXDimension() const;

  /// Determine whetether a valid y dimension has been provided.
  bool hasYDimension() const;

  /// Determine whetether a valid z dimension has been provided.
  bool hasZDimension() const;

  /// Determine whetether a valid t dimension has been provided.
  bool hasTDimension() const;

};

}
}

#endif /* GEOMETRYXMLBUILDER_H_ */
