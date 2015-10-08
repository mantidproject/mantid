#ifndef OBJCOMPASSEMBLY_ACTOR__H_
#define OBJCOMPASSEMBLY_ACTOR__H_
#include "ICompAssemblyActor.h"

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/V3D.h"

class TexObject;
namespace Mantid{
	namespace Geometry{
    class ObjCompAssembly;
	}
}

/**
  \class  ObjCompAssemblyActor
  \brief  This class wraps the ICompAssembly into Actor.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

  This class has the implementation for calling the children of ICompAssembly's IObjComponent to render themselves
  and call the ICompAssemblys. This maintains the count of the children for easy lookup.

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
class ObjCompAssemblyActor : public ICompAssemblyActor
{
public:
  /// Constructor
  ObjCompAssemblyActor(const InstrumentActor& instrActor,Mantid::Geometry::ComponentID compID);
  virtual ~ObjCompAssemblyActor();								   ///< Destructor
  virtual std::string type()const {return "ObjCompAssemblyActor";} ///< Type of the GL object
  virtual void draw(bool picking = false)const;  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  //virtual void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const;
  virtual void setColors();
  bool accept(GLActorVisitor& visitor, VisitorAcceptRule rule = VisitAll);
  bool accept(GLActorConstVisitor& visitor, VisitorAcceptRule rule = VisitAll)const;
private:
  void setDetectorColor(unsigned char* data, size_t i,GLColor c)const; ///< set colour to a detector
  void setDataColors() const;
  void setPickColors() const;
  void generateTexture(unsigned char* data, unsigned int& id) const;
  /// Swap between drawing counts and drawing detector code colours
  void swap();
  const unsigned char* getColor(int i)const;

  std::vector<Mantid::detid_t> m_detIDs;     ///< List of Component IDs
  mutable unsigned int m_idData;     ///< OpenGL texture id
  mutable unsigned int m_idPick;     ///< OpenGL texture id
  int m_n;               ///< texture size in one dimension, the other dimension is 1
  unsigned char* m_data; ///< texture colour data
  unsigned char* m_pick_data; ///< texture with detector code colours
  mutable bool m_texturesGenerated; ///< true if the textures have been generated
};

#endif /*OBJCOMPASSEMBLY_ACTOR__H_*/

