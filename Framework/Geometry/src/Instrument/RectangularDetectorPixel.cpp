// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
    : Detector(base, map), m_panel(base->m_panel), m_row(base->m_row),
      m_col(base->m_col) {}

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
    : Detector(name, id, shape, parent), m_panel(panel), m_row(row),
      m_col(col) {
  if (!m_panel)
    throw std::runtime_error("RectangularDetectorPixel::ctor(): pixel " + name +
                             " has no valid RectangularDetector parent.");
}

//----------------------------------------------------------------------------------------------
/** Get the position relative to the parent IComponent (absolute if no parent)
 * This is calculated on-the-fly.
 *
 * @return position relative to the 0,0 point of the parent panel
 */
Kernel::V3D RectangularDetectorPixel::getRelativePos() const {
  if (m_map && hasDetectorInfo())
    return Detector::getRelativePos();

  // Calculate the x,y position
  double x = m_panel->xstart() + double(m_col) * m_panel->xstep();
  double y = m_panel->ystart() + double(m_row) * m_panel->ystep();
  // The parent m_panel is always the unparametrized version,
  // so the xstep() etc. returned are the UNSCALED one.
  if (m_map) {
    // Apply the scaling factors
    if (auto scalex = m_map->get(m_panel, "scalex"))
      x *= scalex->value<double>();
    if (auto scaley = m_map->get(m_panel, "scaley"))
      y *= scaley->value<double>();
  }
  return V3D(x, y, 0);
}

} // namespace Geometry
} // namespace Mantid
