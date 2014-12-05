#ifndef ICOMPASSEMBLY_ACTOR__H_
#define ICOMPASSEMBLY_ACTOR__H_
#include "ComponentActor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/V3D.h"

#include <boost/shared_ptr.hpp>

#include <map>

/**
  \class  ICompAssemblyActor
  \brief  This class wraps the ICompAssembly into Actor.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

  This class has the interface Comp assembly actors.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
*/
namespace Mantid
{
namespace Kernel
{
  class V3D;
}

namespace Geometry
{
  class ICompAssembly;
  class Object;
}

}

class ObjComponentActor;

class ICompAssemblyActor : public ComponentActor
{
public:
  ICompAssemblyActor(const InstrumentActor& instrActor,const Mantid::Geometry::ComponentID& compID); ///< Constructor
  void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const;

  virtual std::string type()const {return "ICompAssemblyActor";} ///< Type of the GL object
  int getNumberOfDetectors() const { return mNumberOfDetectors;}

protected:
  int mNumberOfDetectors;
  Mantid::Kernel::V3D minBoundBox;
  Mantid::Kernel::V3D maxBoundBox;
};

#endif /*GLTRIANGLE_H_*/

