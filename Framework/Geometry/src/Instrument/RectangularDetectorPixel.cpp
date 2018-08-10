#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace Geometry {

/** Constructor for a parametrized Detector
 * @param base: the base (un-parametrized) IComponent
 * @param map: pointer to the ParameterMap
 * */
RectangularDetectorPixel::RectangularDetectorPixel(
    const RectangularDetectorPixel *base, const ParameterMap *map)
    : GridDetectorPixel(base, map) {}

/** Constructor
 *
 * @param name :: The name of the component
 * @param id :: detector ID
 * @param shape ::  A pointer to the object describing the shape of this
 *component
 * @param parent :: parent IComponent (assembly, normally)
 * @param panel :: parent RectangularDetector
 * @param row :: row of the pixel in the panel
 * @param col :: column of the pixel in the panel
 */
RectangularDetectorPixel::RectangularDetectorPixel(
    const std::string &name, int id, boost::shared_ptr<IObject> shape,
    IComponent *parent, RectangularDetector *panel, size_t row, size_t col)
    : GridDetectorPixel(name, id, shape, parent, panel, row, col, 0) {}

} // namespace Geometry
} // namespace Mantid
