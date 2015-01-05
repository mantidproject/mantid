#ifndef IMDDIMENSIONFACTORY_H_
#define IMDDIMENSIONFACTORY_H_

#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Poco {
namespace XML {
class Element;
}
}

namespace Mantid {
namespace Geometry {
/** Creates IMDDimension objects based on input XML.
 *
 *  Copyright &copy; 2010-2014 ISIS Rutherford Appleton Laboratory, NScD Oak
 *Ridge National Laboratory & European Spallation Source
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
 *  File change history is stored at: <https://github.com/mantidproject/mantid>
 *  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

MANTID_GEOMETRY_DLL IMDDimension_sptr
    createDimension(const std::string &dimensionXMLString);
MANTID_GEOMETRY_DLL IMDDimension_sptr
    createDimension(const Poco::XML::Element &dimensionXML);
MANTID_GEOMETRY_DLL IMDDimension_sptr
    createDimension(const std::string &dimensionXMLString, int nBins,
                    coord_t min, coord_t max);
}
}

#endif
