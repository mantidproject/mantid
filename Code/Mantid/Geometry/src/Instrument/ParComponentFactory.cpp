#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/Instrument/ParCompAssembly.h"
#include "MantidGeometry/Instrument/CompAssembly.h" 
#include "MantidGeometry/Instrument/Detector.h" 
#include "MantidGeometry/Instrument/ParDetector.h" 
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ParRectangularDetector.h"
#include "MantidGeometry/Instrument/ParObjComponent.h" 

namespace Mantid
{
namespace Geometry
{

boost::shared_ptr<IComponent> ParComponentFactory::create(boost::shared_ptr<const IComponent> base, const ParameterMap & map)
{
    if (base)
    {
      const Detector* dc = dynamic_cast<const Detector*>(base.get());
      if (dc)
        return boost::shared_ptr<IComponent>(new ParDetector(dc,map));

      const RectangularDetector* rd = dynamic_cast<const RectangularDetector*>(base.get());
      if (rd)
        return boost::shared_ptr<IComponent>(new ParRectangularDetector(rd,map));

      const CompAssembly* ac = dynamic_cast<const CompAssembly*>(base.get());
      if (ac)
        return boost::shared_ptr<IComponent>(new ParCompAssembly(ac,map));

      const ObjComponent* oc = dynamic_cast<const ObjComponent*>(base.get());
      if (oc)
        return boost::shared_ptr<IComponent>(new ParObjComponent(oc,map));

      //must be a component
      const Component* cc = dynamic_cast<const Component*>(base.get());
      if (cc)
        return boost::shared_ptr<IComponent>(new ParametrizedComponent(cc,map));

    }

     return boost::shared_ptr<IComponent>();
}


}
}
