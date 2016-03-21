#include "MantidGeometry/Instrument/StructuredDetectorPixel.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Kernel;

namespace Mantid {
	namespace Geometry {

		/** Constructor for a parametrized Detector
		* @param base: the base (un-parametrized) IComponent
		* @param map: pointer to the ParameterMap
		* */
		StructuredDetectorPixel::StructuredDetectorPixel(
			const StructuredDetectorPixel *base, const ParameterMap *map)
			: Detector(base, map), m_det(base->m_det), m_row(base->m_row),
			m_col(base->m_col) {}

		/** Constructor
		*
		* @param name :: The name of the component
		* @param id :: detector ID
		* @param shape ::  A pointer to the object describing the shape of this
		*component
		* @param parent :: parent IComponent (assembly, normally)
		* @param det :: parent StructuredDetector
		* @param row :: row of the pixel in the panel
		* @param col :: column of the pixel in the panel
		*/
		StructuredDetectorPixel::StructuredDetectorPixel(
			const std::string &name, int id, boost::shared_ptr<Object> shape,
			IComponent *parent, StructuredDetector *det, size_t row, size_t col)
			: Detector(name, id, shape, parent), m_det(det), m_row(row),
			m_col(col) {
			if (!m_det)
				throw std::runtime_error("StructuredDetectorPixel::ctor(): pixel " + name +
					" has no valid StructuredDetector parent.");
		}

		//----------------------------------------------------------------------------------------------
		/** Get the position relative to the parent IComponent (absolute if no parent)
		* This is calculated on-the-fly.
		*
		* @return position relative to the 0,0 point of the parent panel
		*/
		const Kernel::V3D StructuredDetectorPixel::getRelativePos() const {
			auto w = m_det->xpixels() +1;
			// Calculate the x,y position
			double x = m_det->getXValues().at((m_row * w) + m_col);
			double y = m_det->getYValues().at((m_row * w) + m_col);

			// The parent m_det is always the unparametrized version,
			// so the xpixels() etc. returned are the UNSCALED one.
			if (m_map) {
				// Apply the scaling factors
				if (m_map->contains(m_det, "scalex"))
					x *= m_map->get(m_det, "scalex")->value<double>();
				if (m_map->contains(m_det, "scaley"))
					y *= m_map->get(m_det, "scaley")->value<double>();
			}

			return V3D(x, y, 0);
		}

	} // namespace Mantid
} // namespace Geometry
