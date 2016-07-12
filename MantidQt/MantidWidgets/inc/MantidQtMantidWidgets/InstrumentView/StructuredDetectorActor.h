#ifndef STRUCTUREDDETECTORACTOR
#define STRUCTUREDDETECTORACTOR
#include "GLActor.h"
#include "ICompAssemblyActor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidKernel/V3D.h"
#include "ObjComponentActor.h"
/**
\class  StructuredDetectorActor
\brief  This class wraps a StructuredDetector into Actor.
\author Lamar Moore
\date   March 9 2016
\version 1.0

This class is used to render a StructuredDetector as a bitmap and plot it.

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class ICompAssembly;
class Object;
}
}

namespace MantidQt {
namespace MantidWidgets {
class ObjComponentActor;

class StructuredDetectorActor : public ICompAssemblyActor {
public:
  /// Constructor
  StructuredDetectorActor(const InstrumentActor &instrActor,
                          const Mantid::Geometry::ComponentID &compID);
  /// Destructor
  ~StructuredDetectorActor() override;

private:
  void AppendBoundingBox(const Mantid::Kernel::V3D &minBound,
                         const Mantid::Kernel::V3D &maxBound);

protected:
  /// The structured detector
  boost::shared_ptr<const Mantid::Geometry::StructuredDetector> m_det;
  std::vector<GLColor> m_clist;
  std::vector<size_t> m_pickIds;
  std::vector<GLColor> m_pickColors;

  void init() const;
  void redraw();
  int findDetectorIDUsingColor(int rgb);
  virtual void initChilds(bool) {}

public:
  std::string type() const override {
    return "StructuredDetectorActor";
  } ///< Type of the GL object

  void draw(bool picking = false) const override; ///< Method that
                                                  /// defines
  /// ObjComponent
  /// geometry. Calls
  /// ObjComponent
  /// draw method
  void getBoundingBox(Mantid::Kernel::V3D &minBound,
                      Mantid::Kernel::V3D &maxBound) const override;
  bool accept(GLActorVisitor &visitor,
              VisitorAcceptRule rule = VisitAll) override;
  bool accept(GLActorConstVisitor &visitor,
              VisitorAcceptRule rule = VisitAll) const override;
  bool isChildDetector(const Mantid::Geometry::ComponentID &id) const;
  void setColors() override;
};

} // MantidWidgets
} // MantidQt

#endif // STRUCTUREDDETECTORACTOR