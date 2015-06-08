/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below Geometry
 *    (e.g. Kernel).
 *  Conversely, this file (and its cpp) MAY NOT be modified to use anything from
 *a
 *  package higher than Geometry (e.g. API, DataObjects, ...)
 *********************************************************************************/
#ifndef COMPONENTCREATIONHELPER_H_
#define COMPONENTCREATIONHELPER_H_

#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"

// Forward declarations
namespace Mantid {
namespace Geometry {
class CompAssembly;
class ObjComponent;
class DetectorGroup;
class DetectorsRing;
}
}

namespace ComponentCreationHelper {

/**
A set of helper functions for creating various component structures for the unit
tests.

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

//----------------------------------------------------------------------------------------------
/**
 * Create a capped cylinder object
 */
Mantid::Geometry::Object_sptr
createCappedCylinder(double radius, double height,
                     const Mantid::Kernel::V3D &baseCentre,
                     const Mantid::Kernel::V3D &axis, const std::string &id);
/**
 * Return the XML for a sphere.
 */
std::string sphereXML(double radius, const Mantid::Kernel::V3D &centre,
                      const std::string &id);
/**
 * Create a sphere object
 */
Mantid::Geometry::Object_sptr
createSphere(double radius,
             const Mantid::Kernel::V3D &centre = Mantid::Kernel::V3D(),
             const std::string &id = "sp-1");
/** Create a cuboid shape for your pixels */
Mantid::Geometry::Object_sptr createCuboid(double x_side_length,
                                           double y_side_length = -1.0,
                                           double z_side_length = -1.0);
/**
* Create a component assembly at the origin made up of 4 cylindrical detectors
*/
boost::shared_ptr<Mantid::Geometry::CompAssembly>
createTestAssemblyOfFourCylinders();
/**
 * Create an object component that has a defined shape
 */
Mantid::Geometry::ObjComponent *createSingleObjectComponent();
/**
 * Create a hollow shell, i.e. the intersection of two spheres or radius r1 and
 * r2
 */
Mantid::Geometry::Object_sptr
createHollowShell(double innerRadius, double outerRadius,
                  const Mantid::Kernel::V3D &centre = Mantid::Kernel::V3D());
/**
 * Create a detector group containing 5 detectors
 */
boost::shared_ptr<Mantid::Geometry::DetectorGroup>
createDetectorGroupWith5CylindricalDetectors();
/**
* Create a detector group containing n detectors with gaps
*/
boost::shared_ptr<Mantid::Geometry::DetectorGroup>
createDetectorGroupWithNCylindricalDetectorsWithGaps(unsigned int nDet = 4,
                                                     double gap = 0.01);
/**
 * Create a detector group containing detectors ring
 * R_min -- min radius of the ring
 * R_max -- max radius of the ring, center has to be in 0 position,
 * z     -- axial z-coordinate of the detectors position;
  The detectors are the cylinders with 1.5cm height and 0.5 cm radius
 */
boost::shared_ptr<Mantid::Geometry::DetectorGroup>
createRingOfCylindricalDetectors(const double R_min = 4.5,
                                 const double R_max = 5, const double z = 4);
/**
 * Create a group of two monitors
 */
boost::shared_ptr<Mantid::Geometry::DetectorGroup> createGroupOfTwoMonitors();
/** create instrument with cylindrical detectors located in specific angular
 * positions */
Mantid::Geometry::Instrument_sptr
createCylInstrumentWithDetInGivenPositions(const std::vector<double> &L2,
                                           const std::vector<double> &polar,
                                           const std::vector<double> &azim);
/**
 * Create an test instrument with n panels of 9 cylindrical detectors, a source
 * and spherical sample shape.
 * Detectors have IDs assigned as follows:
 * 7 8 9
 * 4 5 6
 * 1 2 3
 * @param num_banks :: number of 9-cylinder banks to create
 * @param verbose :: prints out the instrument after creation.
 * @param cylRadius :: radius of each detector
 * @param cylHeight :: height of each detector
 * @return Created instrument
 */
Mantid::Geometry::Instrument_sptr
createTestInstrumentCylindrical(int num_banks, bool verbose = false,
                                const double cylRadius = 0.004,
                                const double cylHeight = 0.0002);
/// Create a test instrument with n panels of rectangular detectors,
/// pixels*pixels in size, a source and spherical sample shape.
Mantid::Geometry::Instrument_sptr
createTestInstrumentRectangular(int num_banks, int pixels,
                                double pixelSpacing = 0.008, double bankDistanceFromSample= 5.0);

Mantid::Geometry::Instrument_sptr
createTestInstrumentRectangular2(int num_banks, int pixels,
                                 double pixelSpacing = 0.008);

/// Creates a mimimal valid virtual instrument.
Mantid::Geometry::Instrument_sptr createMinimalInstrument(const Mantid::Kernel::V3D& sourcePos, const Mantid::Kernel::V3D& samplePos, const Mantid::Kernel::V3D& detectorPos );
}

#endif // COMPONENTCREATIONHELPERS_H_
