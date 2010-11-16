#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/CompAssembly.h" 
#include "MantidGeometry/Instrument/ObjCompAssembly.h" 
#include "MantidGeometry/Instrument/ObjCompAssembly.h" 
#include "MantidGeometry/Instrument/Detector.h" 
#include "MantidGeometry/Instrument/Detector.h" 
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ParRectangularDetector.h"
#include "MantidGeometry/Instrument/ObjComponent.h" 

namespace Mantid
{
namespace Geometry
{

boost::shared_ptr<IComponent> ParComponentFactory::create(boost::shared_ptr<const IComponent> base, 
							  const ParameterMap * map)
{
    if (base)
    {
      const Detector* dc = dynamic_cast<const Detector*>(base.get());
      if (dc)
        return boost::shared_ptr<IComponent>(new Detector(dc,map));

      const RectangularDetector* rd = dynamic_cast<const RectangularDetector*>(base.get());
      if (rd)
        return boost::shared_ptr<IComponent>(new ParRectangularDetector(rd,map));

      const CompAssembly* ac = dynamic_cast<const CompAssembly*>(base.get());
      if (ac)
        return boost::shared_ptr<IComponent>(new CompAssembly(ac,map));

      const ObjCompAssembly* oac = dynamic_cast<const ObjCompAssembly*>(base.get());
      if (oac)
        return boost::shared_ptr<IComponent>(new ObjCompAssembly(oac,map));

      const ObjComponent* oc = dynamic_cast<const ObjComponent*>(base.get());
      if (oc)
        return boost::shared_ptr<IComponent>(new ObjComponent(oc,map));

      //must be a component
      const IComponent* cc = dynamic_cast<const IComponent*>(base.get());
      if (cc)
        return boost::shared_ptr<IComponent>(new Component(cc,map));

    }

     return boost::shared_ptr<IComponent>();
}


}
}
