#ifndef MANTID_GEOMETRY_MDFRAMEFACTORY_H_
#define MANTID_GEOMETRY_MDFRAMEFACTORY_H_

#include "MantidKernel/ChainableFactory.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"

namespace Mantid {
namespace Geometry {

/** MDFrameFactory.h : Chain of repsonsibility factory for the MDFrameFactory

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MDFrameFactory
    : public Kernel::ChainableFactory<MDFrameFactory, MDFrame> {
public:
  virtual ~MDFrameFactory(){}
};

//-----------------------------------------------------------------------
// Derived MDFrameFactory declarations
//-----------------------------------------------------------------------
class DLLExport GeneralFrameFactory : public MDFrameFactory {
private:
  GeneralFrame *createRaw(const std::string &argument) const;
public:
  bool canInterpret(const std::string &) const;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MDFRAMEFACTORY_H_ */
