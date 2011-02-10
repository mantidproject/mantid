#ifndef DIMENSIONFACTORY_H_
#define DIMENSIONFACTORY_H_

/**
*  DimensionFactory. Handles conversion of dimension xml to IMDDimension objects.
*
*  This algorithm performs dynamic rebinning driven by the xml string passed as an input.
*
*  @date 10/02/2011
*  @author Owen Arnold
*
*  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
*
*  This file is part of Mantid.
*
*  Mantid is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*
*  Mantid is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Poco
{
namespace XML
{
class Element;
}
}

namespace Mantid
{
namespace Geometry
{
class MDDimension;
}
namespace MDAlgorithms
{

class DimensionFactory
{

public:

  /// Constructor
  DimensionFactory(Poco::XML::Element* dimensionXML);

  /// Constructor
  DimensionFactory(const DimensionFactory& other);

  /// Assignment operator
  DimensionFactory& operator=(const DimensionFactory& other);

  /// Alternate Constructional method.
  static DimensionFactory createDimensionFactory(const std::string& xmlString);

  /// Destructor
  ~DimensionFactory();

  /// Factory method.
  Mantid::Geometry::IMDDimension* create() const;

  /// Factory method. More explicit naming as create() should be preferred.
  Mantid::Geometry::MDDimension* createAsMDDimension() const;

private:

  DimensionFactory();

  void setXMLString(const std::string& xmlString);

  /// Dimension xml to process.
  Poco::XML::Element* m_dimensionXML;

  /// Create an instance of a dimension of the correct type (reciprocal or otherwise)
  Mantid::Geometry::MDDimension* createRawDimension(Poco::XML::Element* reciprocalMapping, const std::string& id) const;
};

}
}

#endif
