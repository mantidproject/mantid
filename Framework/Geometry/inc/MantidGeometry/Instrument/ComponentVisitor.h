#ifndef MANTID_GEOMETRY_COMPONENTVISITOR_H
#define MANTID_GEOMETRY_COMPONENTVISITOR_H

#include <cstddef>
namespace Mantid {
namespace Geometry {

class ICompAssembly;
class IDetector;
class IComponent;

/** ComponentVisitor : Visitor for IComponents. Enables parsing of a full doubly
  linked InstrumentTree without need for dynamic casts. Public methods are
  called by
  accepting IComponent

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class ComponentVisitor {
public:
  virtual void registerComponentAssembly(const ICompAssembly &assembly) = 0;
  virtual void registerGenericComponent(const IComponent &component) = 0;
  virtual void registerDetector(const IDetector &detector) = 0;
  virtual ~ComponentVisitor() {}
};
}
}
#endif
