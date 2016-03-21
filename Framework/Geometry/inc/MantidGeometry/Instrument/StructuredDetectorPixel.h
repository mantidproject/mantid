#ifndef MANTID_GEOMETRY_STRUCTUREDDETECTORPIXEL_H_
#define MANTID_GEOMETRY_STRUCTUREDDETECTORPIXEL_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
	namespace Geometry {

		// Forward declaration
		class StructureedDetector;

		/** StructuredDetectorPixel: a sub-class of Detector
		that is one pixel inside a StructuredDetector.

		The position of the pixel is calculated using the pixel index
		of the parent x and y values(which is parametrized).

		@Author Lamar Moore
		@date 2016-03-17

		Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
		Code Documentation is available at: <http://doxygen.mantidproject.org>
		*/
		class DLLExport StructuredDetectorPixel : public Detector {
			friend class StructuredDetector;

		public:
			/// A string representation of the component type
			std::string type() const override { return "StructuredDetectorPixel"; }

			/// Constructor for parametrized version
			StructuredDetectorPixel(const StructuredDetectorPixel *base,
				const ParameterMap *map);
			StructuredDetectorPixel(const std::string &name, int id,
				boost::shared_ptr<Object> shape, IComponent *parent,
				StructuredDetector *det, size_t row, size_t col);

			StructuredDetectorPixel();

			const Kernel::V3D getRelativePos() const override;

		protected:
			/// StructuredDetector that is the parent of this pixel.
			StructuredDetector *m_det;
			/// Row of the pixel in the panel (y index)
			size_t m_row;
			/// Column of the pixel in the panel (x index)
			size_t m_col;
		};

	} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_STRUCTUREDDETECTORPIXEL_H_ */
