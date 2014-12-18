#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid {
namespace Geometry {

/**
 * Creates an initialized instance of the desired scatterer class
 *
 * This method tries to construct an instance of the class specified by the
 *"name"-parameter.
 * If it is not found, an exception is thrown (see DynamicFactory::create).
 *Otherwise,
 * the object is initialized. If the second argument is not empty, it is
 *expected
 * to contain a semi-colon separated list of "name=value"-pairs. These pairs
 *need to be
 * valid input for assigning properties of the created scatterer. See the
 *example in
 * the general class documentation.
 *
 * @param name :: Class name to construct.
 * @param properties :: Semi-colon separated "name=value"-pairs.
 * @return Initialized scatterer object.
 */
BraggScatterer_sptr
BraggScattererFactoryImpl::createScatterer(const std::string &name,
                                           const std::string &properties) {
  BraggScatterer_sptr scatterer = create(name);
  scatterer->initialize();

  if (!properties.empty()) {
    scatterer->setProperties(properties);
  }

  return scatterer;
}

/// Private constructor.
BraggScattererFactoryImpl::BraggScattererFactoryImpl() {
  Kernel::LibraryManager::Instance();
}

} // namespace Geometry
} // namespace Mantid
