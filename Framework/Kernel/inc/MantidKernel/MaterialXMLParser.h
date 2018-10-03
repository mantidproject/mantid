#ifndef MANTID_KERNEL_MATERIALXMLPARSER_H_
#define MANTID_KERNEL_MATERIALXMLPARSER_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Material.h"
#include <istream>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Kernel {

/**
  Read an XML definition of a Material and produce a new Material object

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL MaterialXMLParser {
public:
  static constexpr const char *MATERIAL_TAG = "material";

  Material parse(std::istream &istr) const;
  Material parse(Poco::XML::Element *node) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MATERIALXMLPARSER_H_ */
