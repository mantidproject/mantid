// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/GridDetectorPixel.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include <memory>

namespace Mantid::Geometry {

/**
 * Create a parameterized detector from the given base detector and
 * ParameterMap. This version
 * avoids a cast by directly returning the Detector pointer
 * @param base A pointer to the unparameterized version
 * @param map A pointer to the ParameterMap
 * @returns A pointer to a parameterized component
 */
std::shared_ptr<IDetector> ParComponentFactory::createDetector(const IDetector *base, const ParameterMap *map) {
  // Clone may be a Detector or GridDetectorPixel instance (or nullptr)
  auto clone = base->cloneParameterized(map);
  return std::shared_ptr<IDetector>(clone);
}

/**
 * Create a parameterized instrument from the given base detector and
 * ParameterMap. This version
 * avoids a cast by directly returning the Instrument pointer
 * @param base A pointer to the unparameterized version
 * @param map A pointer to the ParameterMap
 * @returns A pointer to a parameterized component
 */
std::shared_ptr<Instrument> ParComponentFactory::createInstrument(const std::shared_ptr<const Instrument> &base,
                                                                  const std::shared_ptr<ParameterMap> &map) {
  return std::make_shared<Instrument>(base, map);
}

/**
 * Create a parameterized component from the given base component and
 * ParameterMap
 * SLOW VERSION as it has to check each possible type
 * @param base A pointer to the unparameterized version
 * @param map A pointer to the ParameterMap
 * @returns A pointer to a parameterized component
 */
IComponent_sptr ParComponentFactory::create(const std::shared_ptr<const IComponent> &base, const ParameterMap *map) {
  std::shared_ptr<const IDetector> det_sptr = std::dynamic_pointer_cast<const IDetector>(base);
  if (det_sptr) {
    return createDetector(det_sptr.get(), map);
  }

  std::shared_ptr<const Instrument> inst_sptr = std::dynamic_pointer_cast<const Instrument>(base);
  // @todo One of the review tasks is to take a look at the parameterized mess
  // and
  // short out this problem with different classes carrying different types of
  // pointers around
  if (inst_sptr) {
    return createInstrument(std::const_pointer_cast<Instrument>(inst_sptr),
                            std::shared_ptr<ParameterMap>(const_cast<ParameterMap *>(map), [](auto*){}));
  }

  // Everything gets created on the fly. Note that the order matters here
  // @todo Really could do with a better system than this. Virtual function
  // maybe?
  const auto *sd = dynamic_cast<const StructuredDetector *>(base.get());
  if (sd)
    return std::make_shared<StructuredDetector>(sd, map);

  const auto *rd = dynamic_cast<const RectangularDetector *>(base.get());
  if (rd)
    return std::make_shared<RectangularDetector>(rd, map);

  const auto *gd = dynamic_cast<const GridDetector *>(base.get());
  if (gd)
    return std::make_shared<GridDetector>(gd, map);

  const auto *ac = dynamic_cast<const CompAssembly *>(base.get());
  if (ac)
    return std::make_shared<CompAssembly>(ac, map);
  const auto *oac = dynamic_cast<const ObjCompAssembly *>(base.get());
  if (oac)
    return std::make_shared<ObjCompAssembly>(oac, map);

  const auto *oc = dynamic_cast<const ObjComponent *>(base.get());
  if (oc)
    return std::make_shared<ObjComponent>(oc, map);
  // Must be a component
  const auto *cc = dynamic_cast<const IComponent *>(base.get());
  if (cc)
    return std::make_shared<Component>(cc, map);

  return IComponent_sptr();
}
} // namespace Mantid::Geometry
