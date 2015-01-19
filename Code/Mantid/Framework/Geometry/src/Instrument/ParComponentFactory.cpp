#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"

namespace Mantid {
namespace Geometry {

/**
 * Create a parameterized detector from the given base detector and
 * ParameterMap. This version
 * avoids a cast by directly returning the Detector pointer
 * @param base A pointer to the unparameterized version
 * @param map A pointer to the ParameterMap
 * @returns A pointer to a parameterized component
 */
boost::shared_ptr<Detector>
ParComponentFactory::createDetector(const IDetector *base,
                                    const ParameterMap *map) {
  // RectangularDetectorPixel subclasses Detector so it has to be checked
  // before.
  const RectangularDetectorPixel *rdp =
      dynamic_cast<const RectangularDetectorPixel *>(base);
  if (rdp)
    return boost::shared_ptr<Detector>(new RectangularDetectorPixel(rdp, map));

  const Detector *baseDet = dynamic_cast<const Detector *>(base);
  if (baseDet)
    return boost::shared_ptr<Detector>(
        new Detector(baseDet, map)); // g_detPool.create(baseDet,map);

  return boost::shared_ptr<Detector>();
}

/**
 * Create a parameterized instrument from the given base detector and
 * ParameterMap. This version
 * avoids a cast by directly returning the Instrument pointer
 * @param base A pointer to the unparameterized version
 * @param map A pointer to the ParameterMap
 * @returns A pointer to a parameterized component
 */
boost::shared_ptr<Instrument>
ParComponentFactory::createInstrument(boost::shared_ptr<const Instrument> base,
                                      boost::shared_ptr<ParameterMap> map) {
  return boost::shared_ptr<Instrument>(new Instrument(base, map));
}

/**
 * Create a parameterized component from the given base component and
 * ParameterMap
 * SLOW VERSION as it has to check each possible type
 * @param base A pointer to the unparameterized version
 * @param map A pointer to the ParameterMap
 * @returns A pointer to a parameterized component
 */
IComponent_sptr
ParComponentFactory::create(boost::shared_ptr<const IComponent> base,
                            const ParameterMap *map) {
  // RectangularDetectorPixel subclasses Detector so it has to be checked
  // before.
  const RectangularDetectorPixel *rdp =
      dynamic_cast<const RectangularDetectorPixel *>(base.get());
  if (rdp)
    return boost::shared_ptr<IComponent>(
        new RectangularDetectorPixel(rdp, map));

  boost::shared_ptr<const IDetector> det_sptr =
      boost::dynamic_pointer_cast<const IDetector>(base);
  if (det_sptr) {
    return createDetector(det_sptr.get(), map);
  }

  boost::shared_ptr<const Instrument> inst_sptr =
      boost::dynamic_pointer_cast<const Instrument>(base);
  // @todo One of the review tasks is to take a look at the parameterized mess
  // and
  // short out this problem with different classes carrying different types of
  // pointers around
  if (inst_sptr) {
    return createInstrument(boost::const_pointer_cast<Instrument>(inst_sptr),
                            boost::shared_ptr<ParameterMap>(
                                const_cast<ParameterMap *>(map), NoDeleting()));
  }

  // Everything gets created on the fly. Note that the order matters here
  // @todo Really could do with a better system than this. Virtual function
  // maybe?
  const RectangularDetector *rd =
      dynamic_cast<const RectangularDetector *>(base.get());
  if (rd)
    return boost::shared_ptr<IComponent>(new RectangularDetector(rd, map));

  const CompAssembly *ac = dynamic_cast<const CompAssembly *>(base.get());
  if (ac)
    return boost::shared_ptr<IComponent>(new CompAssembly(ac, map));
  const ObjCompAssembly *oac =
      dynamic_cast<const ObjCompAssembly *>(base.get());
  if (oac)
    return boost::shared_ptr<IComponent>(new ObjCompAssembly(oac, map));

  const ObjComponent *oc = dynamic_cast<const ObjComponent *>(base.get());
  if (oc)
    return boost::shared_ptr<IComponent>(new ObjComponent(oc, map));
  // Must be a component
  const IComponent *cc = dynamic_cast<const IComponent *>(base.get());
  if (cc)
    return boost::shared_ptr<IComponent>(new Component(cc, map));

  return IComponent_sptr();
}
}
}
